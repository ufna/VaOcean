// Copyright 2014 Vladimir Alyamkin. All Rights Reserved.

#include "VaOceanPluginPrivatePCH.h"

#define HALF_SQRT_2	0.7071068f
#define GRAV_ACCEL	981.0f	// The acceleration of gravity, cm/s^2

#define BLOCK_SIZE_X 16
#define BLOCK_SIZE_Y 16


//////////////////////////////////////////////////////////////////////////
// Height map generation helpers

/** Generating gaussian random number with mean 0 and standard deviation 1 */
float Gauss()
{
	float u1 = FMath::SRand();
	float u2 = FMath::SRand();

	if (u1 < 1e-6f)
	{
		u1 = 1e-6f;
	}

	return sqrtf(-2 * logf(u1)) * cosf(2 * PI * u2);
}

/**
 * Phillips Spectrum
 * K: normalized wave vector, W: wind direction, v: wind velocity, a: amplitude constant
 */
float Phillips(FVector2D K, FVector2D W, float v, float a, float dir_depend)
{
	// Largest possible wave from constant wind of velocity v
	float l = v * v / GRAV_ACCEL;

	// Damp out waves with very small length w << l
	float w = l / 1000;

	float Ksqr = K.X * K.X + K.Y * K.Y;
	float Kcos = K.X * W.X + K.Y * W.Y;
	float phillips = a * expf(-1 / (l * l * Ksqr)) / (Ksqr * Ksqr * Ksqr) * (Kcos * Kcos);

	// Filter out waves moving opposite to wind
	if (Kcos < 0)
	{
		phillips *= dir_depend;
	}

	// Damp out waves with very small length w << l
	return phillips * expf(-Ksqr * w * w);
}


//////////////////////////////////////////////////////////////////////////
// Spectrum component

UVaOceanSimulatorComponent::UVaOceanSimulatorComponent(const class FPostConstructInitializeProperties& PCIP)
: Super(PCIP)
{
	bAutoActivate = true;
	bWantsInitializeComponent = true;
	PrimaryComponentTick.bCanEverTick = true;
	//PrimaryComponentTick.TickGroup = TG_DuringPhysics;

	// Vertex to draw on render targets
	m_pQuadVB[0].Set(-1.0f, -1.0f, 0.0f, 1.0f);
	m_pQuadVB[1].Set(-1.0f,  1.0f, 0.0f, 1.0f);
	m_pQuadVB[2].Set( 1.0f, -1.0f, 0.0f, 1.0f);
	m_pQuadVB[3].Set( 1.0f,  1.0f, 0.0f, 1.0f);

	// Be sure that there'is no garbede in it
	StateActor = nullptr;
}

void UVaOceanSimulatorComponent::BeginDestroy()
{
	RadixDestroyPlan(&FFTPlan);

	m_pBuffer_Float2_H0.SafeRelease();
	m_pUAV_H0.SafeRelease();
	m_pSRV_H0.SafeRelease();

	m_pBuffer_Float_Omega.SafeRelease();
	m_pUAV_Omega.SafeRelease();
	m_pSRV_Omega.SafeRelease();

	m_pBuffer_Float2_Ht.SafeRelease();
	m_pUAV_Ht.SafeRelease();
	m_pSRV_Ht.SafeRelease();

	m_pBuffer_Float_Dxyz.SafeRelease();
	m_pUAV_Dxyz;
	m_pSRV_Dxyz;

	Super::BeginDestroy();
}

void UVaOceanSimulatorComponent::InitializeComponent()
{
	StateActor = Cast<AVaOceanStateActor>(GetOwner());
	if (StateActor == NULL)
	{
		UE_LOG(LogVaOcean, Warning, TEXT("Simulator component should belong to VaOceanStateActor only!"));
		return;
	}

	// Cache shader immutable parameters (looks ugly, but nicely used then)
	UpdateSpectrumCSImmutableParams.g_ActualDim = StateActor->GetSpectrumConfig().DispMapDimension;
	UpdateSpectrumCSImmutableParams.g_InWidth = UpdateSpectrumCSImmutableParams.g_ActualDim + 4;
	UpdateSpectrumCSImmutableParams.g_OutWidth = UpdateSpectrumCSImmutableParams.g_ActualDim;
	UpdateSpectrumCSImmutableParams.g_OutHeight = UpdateSpectrumCSImmutableParams.g_ActualDim;
	UpdateSpectrumCSImmutableParams.g_DtxAddressOffset = FMath::Square(UpdateSpectrumCSImmutableParams.g_ActualDim);
	UpdateSpectrumCSImmutableParams.g_DtyAddressOffset = FMath::Square(UpdateSpectrumCSImmutableParams.g_ActualDim) * 2;

	// Height map H(0)
	int32 height_map_size = (StateActor->GetSpectrumConfig().DispMapDimension + 4) * (StateActor->GetSpectrumConfig().DispMapDimension + 1);
	TResourceArray<FVector2D> h0_data;
	h0_data.Init(FVector2D::ZeroVector, height_map_size);
	TResourceArray<float> omega_data;
	omega_data.Init(0.0f, height_map_size);
	InitHeightMap(StateActor->GetSpectrumConfig(), h0_data, omega_data);

	int hmap_dim = StateActor->GetSpectrumConfig().DispMapDimension;
	int input_full_size = (hmap_dim + 4) * (hmap_dim + 1);
	// This value should be (hmap_dim / 2 + 1) * hmap_dim, but we use full sized buffer here for simplicity.
	int input_half_size = hmap_dim * hmap_dim;
	int output_size = hmap_dim * hmap_dim;

	// For filling the buffer with zeroes
	TResourceArray<float> zero_data;
	zero_data.Init(0.0f, 3 * output_size * 2);

	// RW buffer allocations
	// H0
	uint32 float2_stride = 2 * sizeof(float);
	CreateBufferAndUAV(&h0_data, input_full_size * float2_stride, float2_stride, &m_pBuffer_Float2_H0, &m_pUAV_H0, &m_pSRV_H0);

	// Notice: The following 3 buffers should be half sized buffer because of conjugate symmetric input. But
	// we use full sized buffers due to the CS4.0 restriction.

	// Put H(t), Dx(t) and Dy(t) into one buffer because CS4.0 allows only 1 UAV at a time
	CreateBufferAndUAV(&zero_data, 3 * input_half_size * float2_stride, float2_stride, &m_pBuffer_Float2_Ht, &m_pUAV_Ht, &m_pSRV_Ht);

	// omega
	CreateBufferAndUAV(&omega_data, input_full_size * sizeof(float), sizeof(float), &m_pBuffer_Float_Omega, &m_pUAV_Omega, &m_pSRV_Omega);

	// Re-init the array because it was discarded by previous buffer creation
	zero_data.Empty();
	zero_data.Init(0.0f, 3 * output_size * 2);
	// Notice: The following 3 should be real number data. But here we use the complex numbers and C2C FFT
	// due to the CS4.0 restriction.
	// Put Dz, Dx and Dy into one buffer because CS4.0 allows only 1 UAV at a time
	CreateBufferAndUAV(&zero_data, 3 * output_size * float2_stride, float2_stride, &m_pBuffer_Float_Dxyz, &m_pUAV_Dxyz, &m_pSRV_Dxyz);

	// FFT
	RadixCreatePlan(&FFTPlan, 3);
}

void UVaOceanSimulatorComponent::InitHeightMap(const FSpectrumData& Params, TResourceArray<FVector2D>& out_h0, TResourceArray<float>& out_omega)
{
	int32 i, j;
	FVector2D K, Kn;

	FVector2D wind_dir = Params.WindDirection;
	wind_dir.Normalize();

	float a = Params.WaveAmplitude * 1e-7f;	// It is too small. We must scale it for editing.
	float v = Params.WindSpeed;
	float dir_depend = Params.WindDependency;

	int height_map_dim = Params.DispMapDimension;
	float patch_length = Params.PatchLength;

	for (i = 0; i <= height_map_dim; i++)
	{
		// K is wave-vector, range [-|DX/W, |DX/W], [-|DY/H, |DY/H]
		K.Y = (-height_map_dim / 2.0f + i) * (2 * PI / patch_length);

		for (j = 0; j <= height_map_dim; j++)
		{
			K.X = (-height_map_dim / 2.0f + j) * (2 * PI / patch_length);

			float phil = (K.X == 0 && K.Y == 0) ? 0 : sqrtf(Phillips(K, wind_dir, v, a, dir_depend));

			out_h0[i * (height_map_dim + 4) + j].X = float(phil * Gauss() * HALF_SQRT_2);
			out_h0[i * (height_map_dim + 4) + j].Y = float(phil * Gauss() * HALF_SQRT_2);

			// The angular frequency is following the dispersion relation:
			//            out_omega^2 = g*k
			// The equation of Gerstner wave:
			//            x = x0 - K/k * A * sin(dot(K, x0) - sqrt(g * k) * t), x is a 2D vector.
			//            z = A * cos(dot(K, x0) - sqrt(g * k) * t)
			// Gerstner wave shows that a point on a simple sinusoid wave is doing a uniform circular
			// motion with the center (x0, y0, z0), radius A, and the circular plane is parallel to
			// vector K.
			out_omega[i * (height_map_dim + 4) + j] = sqrtf(GRAV_ACCEL * sqrtf(K.X * K.X + K.Y * K.Y));
		}
	}
}

void UVaOceanSimulatorComponent::CreateBufferAndUAV(FResourceArrayInterface* Data, uint32 byte_width, uint32 byte_stride,
	FStructuredBufferRHIRef* ppBuffer, FUnorderedAccessViewRHIRef* ppUAV, FShaderResourceViewRHIRef* ppSRV)
{
	*ppBuffer = RHICreateStructuredBuffer(byte_stride, Data->GetResourceDataSize(), Data, (BUF_UnorderedAccess | BUF_ShaderResource));
	*ppUAV = RHICreateUnorderedAccessView(*ppBuffer, false, false);
	*ppSRV = RHICreateShaderResourceView(*ppBuffer);
}


//////////////////////////////////////////////////////////////////////////
// Ocean simulation (displacement map update)

#if WITH_EDITOR
void UVaOceanSimulatorComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	// AActor::PostEditChange will ForceUpdateComponents()
	Super::PostEditChangeProperty(PropertyChangedEvent);

	//UpdateContent();
}
#endif // WITH_EDITOR

void UVaOceanSimulatorComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	UpdateContent();
}

void UVaOceanSimulatorComponent::UpdateContent()
{
	UpdateDisplacementMap(GetWorld()->GetTimeSeconds());
	UpdateDisplacementArray();
}

/** Vertex declaration for the fullscreen 2D quad */
TGlobalResource<FQuadVertexDeclaration> GQuadVertexDeclaration;

void UVaOceanSimulatorComponent::UpdateDisplacementMap(float WorldTime)
{
	if (StateActor == NULL || DisplacementTarget == NULL || GradientTarget == NULL)
		return;
	
	// ---------------------------- H(0) -> H(t), D(x, t), D(y, t) --------------------------------
	FUpdateSpectrumCSPerFrame UpdateSpectrumCSPerFrameParams;
	UpdateSpectrumCSPerFrameParams.g_Time = WorldTime * StateActor->GetSpectrumConfig().TimeScale;
	UpdateSpectrumCSPerFrameParams.g_ChoppyScale = StateActor->GetSpectrumConfig().ChoppyScale;
	UpdateSpectrumCSPerFrameParams.m_pSRV_H0 = m_pSRV_H0;
	UpdateSpectrumCSPerFrameParams.m_pSRV_Omega = m_pSRV_Omega;
	UpdateSpectrumCSPerFrameParams.m_pUAV_Ht = m_pUAV_Ht;

	ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER(
		UpdateSpectrumCSCommand,
		FUpdateSpectrumCSImmutable, ImmutableParams, UpdateSpectrumCSImmutableParams,
		FUpdateSpectrumCSPerFrame, PerFrameParams, UpdateSpectrumCSPerFrameParams,
		{
			FUpdateSpectrumUniformParameters Parameters;
			Parameters.Time = PerFrameParams.g_Time;

			FUpdateSpectrumUniformBufferRef UniformBuffer = 
				FUpdateSpectrumUniformBufferRef::CreateUniformBufferImmediate(Parameters, UniformBuffer_SingleUse);

			TShaderMapRef<FUpdateSpectrumCS> UpdateSpectrumCS(GetGlobalShaderMap());
			RHISetComputeShader(UpdateSpectrumCS->GetComputeShader());

			UpdateSpectrumCS->SetParameters(ImmutableParams.g_ActualDim,
				ImmutableParams.g_InWidth, ImmutableParams.g_OutWidth, ImmutableParams.g_OutHeight,
				ImmutableParams.g_DtxAddressOffset, ImmutableParams.g_DtyAddressOffset);

			UpdateSpectrumCS->SetParameters(UniformBuffer, PerFrameParams.m_pSRV_H0, PerFrameParams.m_pSRV_Omega);
			UpdateSpectrumCS->SetOutput(PerFrameParams.m_pUAV_Ht);

			uint32 group_count_x = (ImmutableParams.g_ActualDim + BLOCK_SIZE_X - 1) / BLOCK_SIZE_X;
			uint32 group_count_y = (ImmutableParams.g_ActualDim + BLOCK_SIZE_Y - 1) / BLOCK_SIZE_Y;
			RHIDispatchComputeShader(group_count_x, group_count_y, 1);

			UpdateSpectrumCS->UnsetParameters();
			UpdateSpectrumCS->UnbindBuffers();
		});

	// ------------------------------------ Perform FFT -------------------------------------------
	ENQUEUE_UNIQUE_RENDER_COMMAND_FOURPARAMETER(
		RadixFFTCommand,
		FRadixPlan512*, pPlan, &FFTPlan,
		FUnorderedAccessViewRHIRef, m_pUAV_Dxyz, m_pUAV_Dxyz,
		FShaderResourceViewRHIRef, m_pSRV_Dxyz, m_pSRV_Dxyz,
		FShaderResourceViewRHIRef, m_pSRV_Ht, m_pSRV_Ht,
		{
			RadixCompute(pPlan, m_pUAV_Dxyz, m_pSRV_Dxyz, m_pSRV_Ht);
		});

	// --------------------------------- Wrap Dx, Dy and Dz ---------------------------------------
	FUpdateDisplacementPSPerFrame UpdateDisplacementPSPerFrameParams;
	UpdateDisplacementPSPerFrameParams.g_ChoppyScale = StateActor->GetSpectrumConfig().ChoppyScale;
	UpdateDisplacementPSPerFrameParams.g_GridLen = StateActor->GetSpectrumConfig().DispMapDimension / StateActor->GetSpectrumConfig().PatchLength;
	UpdateDisplacementPSPerFrameParams.g_InputDxyz = m_pSRV_Dxyz;
	FMemory::Memcpy(UpdateDisplacementPSPerFrameParams.m_pQuadVB, m_pQuadVB, sizeof(m_pQuadVB[0]) * 4);

	FTextureRenderTargetResource* DisplacementRenderTarget = DisplacementTarget->GameThread_GetRenderTargetResource();

	ENQUEUE_UNIQUE_RENDER_COMMAND_THREEPARAMETER(
		UpdateDisplacementPSCommand,
		FTextureRenderTargetResource*, TextureRenderTarget, DisplacementRenderTarget,
		FUpdateSpectrumCSImmutable, ImmutableParams, UpdateSpectrumCSImmutableParams,		// We're using the same params as for CS
		FUpdateDisplacementPSPerFrame, PerFrameParams, UpdateDisplacementPSPerFrameParams,
		{
			FUpdateDisplacementUniformParameters Parameters;
			Parameters.ChoppyScale = PerFrameParams.g_ChoppyScale;
			Parameters.GridLen = PerFrameParams.g_GridLen;

			FUpdateDisplacementUniformBufferRef UniformBuffer =
				FUpdateDisplacementUniformBufferRef::CreateUniformBufferImmediate(Parameters, UniformBuffer_SingleUse);

			RHISetRenderTarget(TextureRenderTarget->GetRenderTargetTexture(), NULL);
			RHIClear(true, FLinearColor::Transparent, false, 0.f, false, 0, FIntRect());

			TShaderMapRef<FQuadVS> QuadVS(GetGlobalShaderMap());
			TShaderMapRef<FUpdateDisplacementPS> UpdateDisplacementPS(GetGlobalShaderMap());

			static FGlobalBoundShaderState UpdateDisplacementBoundShaderState;
			SetGlobalBoundShaderState(UpdateDisplacementBoundShaderState, GQuadVertexDeclaration.VertexDeclarationRHI, *QuadVS, *UpdateDisplacementPS);

			UpdateDisplacementPS->SetParameters(ImmutableParams.g_ActualDim,
				ImmutableParams.g_InWidth, ImmutableParams.g_OutWidth, ImmutableParams.g_OutHeight,
				ImmutableParams.g_DtxAddressOffset, ImmutableParams.g_DtyAddressOffset);

			UpdateDisplacementPS->SetParameters(UniformBuffer, PerFrameParams.g_InputDxyz);

			RHIDrawPrimitiveUP(PT_TriangleStrip, 2, PerFrameParams.m_pQuadVB, sizeof(PerFrameParams.m_pQuadVB[0]));
			//RHICopyToResolveTarget(TextureRenderTarget->GetRenderTargetTexture(), TextureRenderTarget->TextureRHI, false, FResolveParams());

			UpdateDisplacementPS->UnsetParameters();
		});

	// ----------------------------------- Generate Normal ----------------------------------------
	FGenGradientFoldingPSPerFrame GenGradientFoldingPSPerFrameParams;
	GenGradientFoldingPSPerFrameParams.g_ChoppyScale = StateActor->GetSpectrumConfig().ChoppyScale;
	GenGradientFoldingPSPerFrameParams.g_GridLen = StateActor->GetSpectrumConfig().DispMapDimension / StateActor->GetSpectrumConfig().PatchLength;
	FMemory::Memcpy(GenGradientFoldingPSPerFrameParams.m_pQuadVB, m_pQuadVB, sizeof(m_pQuadVB[0]) * 4);

	FTextureRenderTargetResource* GradientRenderTarget = GradientTarget->GameThread_GetRenderTargetResource();

	ENQUEUE_UNIQUE_RENDER_COMMAND_FOURPARAMETER(
		GenGradientFoldingPSCommand,
		FTextureRenderTargetResource*, TextureRenderTarget, GradientRenderTarget,
		FUpdateSpectrumCSImmutable, ImmutableParams, UpdateSpectrumCSImmutableParams,		// We're using the same params as for CS
		FGenGradientFoldingPSPerFrame, PerFrameParams, GenGradientFoldingPSPerFrameParams,
		FTextureRenderTargetResource*, DisplacementRenderTarget, DisplacementRenderTarget,
		{
			FUpdateDisplacementUniformParameters Parameters;
			Parameters.ChoppyScale = PerFrameParams.g_ChoppyScale;
			Parameters.GridLen = PerFrameParams.g_GridLen;

			FUpdateDisplacementUniformBufferRef UniformBuffer =
				FUpdateDisplacementUniformBufferRef::CreateUniformBufferImmediate(Parameters, UniformBuffer_SingleUse);

			RHISetRenderTarget(TextureRenderTarget->GetRenderTargetTexture(), NULL);
			RHIClear(true, FLinearColor::Transparent, false, 0.f, false, 0, FIntRect());

			TShaderMapRef<FQuadVS> QuadVS(GetGlobalShaderMap());
			TShaderMapRef<FGenGradientFoldingPS> GenGradientFoldingPS(GetGlobalShaderMap());

			static FGlobalBoundShaderState UpdateDisplacementBoundShaderState;
			SetGlobalBoundShaderState(UpdateDisplacementBoundShaderState, GQuadVertexDeclaration.VertexDeclarationRHI, *QuadVS, *GenGradientFoldingPS);

			GenGradientFoldingPS->SetParameters(ImmutableParams.g_ActualDim,
				ImmutableParams.g_InWidth, ImmutableParams.g_OutWidth, ImmutableParams.g_OutHeight,
				ImmutableParams.g_DtxAddressOffset, ImmutableParams.g_DtyAddressOffset);

			GenGradientFoldingPS->SetParameters(UniformBuffer, DisplacementRenderTarget->TextureRHI);

			RHIDrawPrimitiveUP(PT_TriangleStrip, 2, PerFrameParams.m_pQuadVB, sizeof(PerFrameParams.m_pQuadVB[0]));
			//RHICopyToResolveTarget(TextureRenderTarget->GetRenderTargetTexture(), TextureRenderTarget->TextureRHI, false, FResolveParams());

			GenGradientFoldingPS->UnsetParameters();
	});
	
}


//////////////////////////////////////////////////////////////////////////
// Buffered data access API

FLinearColor UVaOceanSimulatorComponent::GetDisplacementColor(int32 X, int32 Y) const
{
	return FLinearColor::Black;
}

FLinearColor UVaOceanSimulatorComponent::GetGradientColor(int32 X, int32 Y) const
{
	return FLinearColor::Black;
}

float UVaOceanSimulatorComponent::GetOceanLevelAtLocation(FVector& Location) const
{
	// TODO: expose these variables
	float WorldUVx = Location.X / 2048.0f;
	float WorldUVy = Location.Y / 2048.0f;

	FFloat16Color PixelColour = GetHeightMapPixelColor(WorldUVx, WorldUVy);

	return PixelColour.B;
}

void UVaOceanSimulatorComponent::UpdateDisplacementArray()
{
	ColorBuffer.Reset();

	if (DisplacementTarget)
	{
		FTextureRenderTarget2DResource* textureResource = (FTextureRenderTarget2DResource*)ResultantTexture->Resource;
		if (textureResource->ReadFloat16Pixels(ColorBuffer))
		{

		}
		else
		{
			if (GEngine)
			{
				GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Yellow, TEXT("Error reading texture resource"));
			}
		}
	}
}

FFloat16Color UVaOceanSimulatorComponent::GetHeightMapPixelColor(float U, float V) const
{
	// Check we have a raw data loaded
	if (ColorBuffer.Num() == 0)
	{
		//UE_LOG(LogVaOcean, Warning, TEXT("Ocean heightmap raw data is not loaded! Pixel is empty."));
		return FFloat16Color(FColor::Black);
	}

#if WITH_EDITORONLY_DATA
	// We are using the source art so grab the original width/height
	FTextureRenderTarget2DResource* textureResource = (FTextureRenderTarget2DResource*)ResultantTexture->Resource;
	const int32 Width = textureResource->GetSizeXY().X;
	const int32 Height = textureResource->GetSizeXY().Y;
	const bool bUseSRGB = ResultantTexture->SRGB;

	check(Width > 0 && Height > 0 && ColorBuffer.Num() > 0);

	// Normalize UV first
	const float NormalizedU = U > 0 ? FMath::Fractional(U) : 1.0 + FMath::Fractional(U);
	const float NormalizedV = V > 0 ? FMath::Fractional(V) : 1.0 + FMath::Fractional(V);

	const int PixelX = NormalizedU * (Width - 1) + 1;
	const int PixelY = NormalizedV * (Height - 1) + 1;

	// Get color from
	return ColorBuffer[(PixelY - 1) * Width + PixelX - 1];
#else
	return FFloat16Color(FColor::Black);
#endif
}

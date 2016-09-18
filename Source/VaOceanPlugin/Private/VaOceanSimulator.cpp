// Copyright 2014-2016 Vladimir Alyamkin. All Rights Reserved.

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
// Phillips spectrum simulator

AVaOceanSimulator::AVaOceanSimulator(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_DuringPhysics;
	bReplicates = true;
	NetUpdateFrequency = 10.f;

	// Vertex to draw on render targets
	m_pQuadVB[0].Set(-1.0f, -1.0f, 0.0f, 1.0f);
	m_pQuadVB[1].Set(-1.0f,  1.0f, 0.0f, 1.0f);
	m_pQuadVB[2].Set( 1.0f, -1.0f, 0.0f, 1.0f);
	m_pQuadVB[3].Set( 1.0f,  1.0f, 0.0f, 1.0f);

	// Initialize simulator now
	InitializeSimulator();
}

void AVaOceanSimulator::InitializeSimulator()
{
	// Cache shader immutable parameters (looks ugly, but nicely used then)
	UpdateSpectrumCSImmutableParams.g_ActualDim = SpectrumConfig.DispMapDimension;
	UpdateSpectrumCSImmutableParams.g_InWidth = UpdateSpectrumCSImmutableParams.g_ActualDim + 4;
	UpdateSpectrumCSImmutableParams.g_OutWidth = UpdateSpectrumCSImmutableParams.g_ActualDim;
	UpdateSpectrumCSImmutableParams.g_OutHeight = UpdateSpectrumCSImmutableParams.g_ActualDim;
	UpdateSpectrumCSImmutableParams.g_DtxAddressOffset = FMath::Square(UpdateSpectrumCSImmutableParams.g_ActualDim);
	UpdateSpectrumCSImmutableParams.g_DtyAddressOffset = FMath::Square(UpdateSpectrumCSImmutableParams.g_ActualDim) * 2;

	// Height map H(0)
	int32 height_map_size = (SpectrumConfig.DispMapDimension + 4) * (SpectrumConfig.DispMapDimension + 1);
	TResourceArray<FVector2D> h0_data;
	h0_data.Init(FVector2D::ZeroVector, height_map_size);
	TResourceArray<float> omega_data;
	omega_data.Init(0.0f, height_map_size);
	InitHeightMap(SpectrumConfig, h0_data, omega_data);

	int hmap_dim = SpectrumConfig.DispMapDimension;
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

void AVaOceanSimulator::InitHeightMap(const FSpectrumData& Params, TResourceArray<FVector2D>& out_h0, TResourceArray<float>& out_omega)
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

void AVaOceanSimulator::CreateBufferAndUAV(FResourceArrayInterface* Data, uint32 byte_width, uint32 byte_stride,
	FStructuredBufferRHIRef* ppBuffer, FUnorderedAccessViewRHIRef* ppUAV, FShaderResourceViewRHIRef* ppSRV)
{
	FRHIResourceCreateInfo ResourceCreateInfo;
	ResourceCreateInfo.ResourceArray = Data;
	*ppBuffer = RHICreateStructuredBuffer(byte_stride, Data->GetResourceDataSize(), (BUF_UnorderedAccess | BUF_ShaderResource), ResourceCreateInfo);

	*ppUAV = RHICreateUnorderedAccessView(*ppBuffer, false, false);
	*ppSRV = RHICreateShaderResourceView(*ppBuffer);
}

void AVaOceanSimulator::BeginDestroy()
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


//////////////////////////////////////////////////////////////////////////
// Simulation

#if WITH_EDITOR
void AVaOceanSimulator::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	// AActor::PostEditChange will ForceUpdateComponents()
	Super::PostEditChangeProperty(PropertyChangedEvent);

	// @todo Update shader configuration
}
#endif // WITH_EDITOR

void AVaOceanSimulator::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	UpdateDisplacementMap(GetWorld()->GetTimeSeconds());
}


/** Vertex declaration for the fullscreen 2D quad */
TGlobalResource<FQuadVertexDeclaration> GQuadVertexDeclaration;

void AVaOceanSimulator::UpdateDisplacementMap(float WorldTime)
{
	if (DisplacementTarget == NULL || GradientTarget == NULL)
		return;
	
	// ---------------------------- H(0) -> H(t), D(x, t), D(y, t) --------------------------------
	FUpdateSpectrumCSPerFrame UpdateSpectrumCSPerFrameParams;
	UpdateSpectrumCSPerFrameParams.g_Time = WorldTime * SpectrumConfig.TimeScale;
	UpdateSpectrumCSPerFrameParams.g_ChoppyScale = SpectrumConfig.ChoppyScale;
	UpdateSpectrumCSPerFrameParams.m_pSRV_H0 = m_pSRV_H0;
	UpdateSpectrumCSPerFrameParams.m_pSRV_Omega = m_pSRV_Omega;
	UpdateSpectrumCSPerFrameParams.m_pUAV_Ht = m_pUAV_Ht;

	ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER(
		UpdateSpectrumCSCommand,
		FUpdateSpectrumCSImmutable, ImmutableParams, UpdateSpectrumCSImmutableParams,
		FUpdateSpectrumCSPerFrame, PerFrameParams, UpdateSpectrumCSPerFrameParams,
		{
			FUpdateSpectrumUniformParameters Parameters;
			const auto FeatureLevel = GMaxRHIFeatureLevel;
			Parameters.Time = PerFrameParams.g_Time;

			FUpdateSpectrumUniformBufferRef UniformBuffer = 
				FUpdateSpectrumUniformBufferRef::CreateUniformBufferImmediate(Parameters, UniformBuffer_SingleFrame);

			TShaderMapRef<FUpdateSpectrumCS> UpdateSpectrumCS(GetGlobalShaderMap(GMaxRHIFeatureLevel));
			RHICmdList.SetComputeShader(UpdateSpectrumCS->GetComputeShader());

			UpdateSpectrumCS->SetParameters(RHICmdList, ImmutableParams.g_ActualDim,
				ImmutableParams.g_InWidth, ImmutableParams.g_OutWidth, ImmutableParams.g_OutHeight,
				ImmutableParams.g_DtxAddressOffset, ImmutableParams.g_DtyAddressOffset);

			UpdateSpectrumCS->SetParameters(RHICmdList, UniformBuffer, PerFrameParams.m_pSRV_H0, PerFrameParams.m_pSRV_Omega);
			UpdateSpectrumCS->SetOutput(RHICmdList, PerFrameParams.m_pUAV_Ht);

			uint32 group_count_x = (ImmutableParams.g_ActualDim + BLOCK_SIZE_X - 1) / BLOCK_SIZE_X;
			uint32 group_count_y = (ImmutableParams.g_ActualDim + BLOCK_SIZE_Y - 1) / BLOCK_SIZE_Y;
			RHICmdList.DispatchComputeShader(group_count_x, group_count_y, 1);

			UpdateSpectrumCS->UnsetParameters(RHICmdList);
			UpdateSpectrumCS->UnbindBuffers(RHICmdList);
		});

	// ------------------------------------ Perform FFT -------------------------------------------
	ENQUEUE_UNIQUE_RENDER_COMMAND_FOURPARAMETER(
		RadixFFTCommand,
		FRadixPlan512*, pPlan, &FFTPlan,
		FUnorderedAccessViewRHIRef, m_pUAV_Dxyz, m_pUAV_Dxyz,
		FShaderResourceViewRHIRef, m_pSRV_Dxyz, m_pSRV_Dxyz,
		FShaderResourceViewRHIRef, m_pSRV_Ht, m_pSRV_Ht,
		{
			RadixCompute(RHICmdList, pPlan, m_pUAV_Dxyz, m_pSRV_Dxyz, m_pSRV_Ht);
		});

	// --------------------------------- Wrap Dx, Dy and Dz ---------------------------------------
	FUpdateDisplacementPSPerFrame UpdateDisplacementPSPerFrameParams;
	UpdateDisplacementPSPerFrameParams.g_ChoppyScale = SpectrumConfig.ChoppyScale;
	UpdateDisplacementPSPerFrameParams.g_GridLen = SpectrumConfig.DispMapDimension / SpectrumConfig.PatchLength;
	UpdateDisplacementPSPerFrameParams.g_InputDxyz = m_pSRV_Dxyz;
	FMemory::Memcpy(UpdateDisplacementPSPerFrameParams.m_pQuadVB, m_pQuadVB, sizeof(m_pQuadVB[0]) * 4);

	FTextureRenderTargetResource* DisplacementRenderTarget = DisplacementTarget->GameThread_GetRenderTargetResource();

	ENQUEUE_UNIQUE_RENDER_COMMAND_THREEPARAMETER(
		UpdateDisplacementPSCommand,
		FTextureRenderTargetResource*, TextureRenderTarget, DisplacementRenderTarget,
		FUpdateSpectrumCSImmutable, ImmutableParams, UpdateSpectrumCSImmutableParams,		// We're using the same params as for CS
		FUpdateDisplacementPSPerFrame, PerFrameParams, UpdateDisplacementPSPerFrameParams,
		{
			const auto FeatureLevel = GMaxRHIFeatureLevel;
			FUpdateDisplacementUniformParameters Parameters;
			Parameters.ChoppyScale = PerFrameParams.g_ChoppyScale;
			Parameters.GridLen = PerFrameParams.g_GridLen;

			FUpdateDisplacementUniformBufferRef UniformBuffer =
				FUpdateDisplacementUniformBufferRef::CreateUniformBufferImmediate(Parameters, UniformBuffer_SingleFrame);

			SetRenderTarget(RHICmdList, TextureRenderTarget->GetRenderTargetTexture(), NULL);
			RHICmdList.Clear(true, FLinearColor::Transparent, false, 0.f, false, 0, FIntRect());

			TShaderMapRef<FQuadVS> QuadVS(GetGlobalShaderMap(GMaxRHIFeatureLevel));
			TShaderMapRef<FUpdateDisplacementPS> UpdateDisplacementPS(GetGlobalShaderMap(GMaxRHIFeatureLevel));

			static FGlobalBoundShaderState UpdateDisplacementBoundShaderState;
			SetGlobalBoundShaderState(RHICmdList, GMaxRHIFeatureLevel, UpdateDisplacementBoundShaderState, GQuadVertexDeclaration.VertexDeclarationRHI, *QuadVS, *UpdateDisplacementPS);

			UpdateDisplacementPS->SetParameters(RHICmdList, ImmutableParams.g_ActualDim,
				ImmutableParams.g_InWidth, ImmutableParams.g_OutWidth, ImmutableParams.g_OutHeight,
				ImmutableParams.g_DtxAddressOffset, ImmutableParams.g_DtyAddressOffset);

			UpdateDisplacementPS->SetParameters(RHICmdList, UniformBuffer, PerFrameParams.g_InputDxyz);

			DrawPrimitiveUP(RHICmdList, PT_TriangleStrip, 2, PerFrameParams.m_pQuadVB, sizeof(PerFrameParams.m_pQuadVB[0]));
			//RHICopyToResolveTarget(TextureRenderTarget->GetRenderTargetTexture(), TextureRenderTarget->TextureRHI, false, FResolveParams());

			UpdateDisplacementPS->UnsetParameters(RHICmdList);
		});

	// ----------------------------------- Generate Normal ----------------------------------------
	FGenGradientFoldingPSPerFrame GenGradientFoldingPSPerFrameParams;
	GenGradientFoldingPSPerFrameParams.g_ChoppyScale = SpectrumConfig.ChoppyScale;
	GenGradientFoldingPSPerFrameParams.g_GridLen = SpectrumConfig.DispMapDimension / SpectrumConfig.PatchLength;
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
			const auto FeatureLevel = GMaxRHIFeatureLevel;
			Parameters.ChoppyScale = PerFrameParams.g_ChoppyScale;
			Parameters.GridLen = PerFrameParams.g_GridLen;

			FUpdateDisplacementUniformBufferRef UniformBuffer =
				FUpdateDisplacementUniformBufferRef::CreateUniformBufferImmediate(Parameters, UniformBuffer_SingleFrame);

			SetRenderTarget(RHICmdList, TextureRenderTarget->GetRenderTargetTexture(), NULL);
			RHICmdList.Clear(true, FLinearColor::Transparent, false, 0.f, false, 0, FIntRect());

			TShaderMapRef<FQuadVS> QuadVS(GetGlobalShaderMap(GMaxRHIFeatureLevel));
			TShaderMapRef<FGenGradientFoldingPS> GenGradientFoldingPS(GetGlobalShaderMap(GMaxRHIFeatureLevel));

			static FGlobalBoundShaderState UpdateDisplacementBoundShaderState;
			SetGlobalBoundShaderState(RHICmdList, GMaxRHIFeatureLevel, UpdateDisplacementBoundShaderState, GQuadVertexDeclaration.VertexDeclarationRHI, *QuadVS, *GenGradientFoldingPS);

			GenGradientFoldingPS->SetParameters(RHICmdList, ImmutableParams.g_ActualDim,
				ImmutableParams.g_InWidth, ImmutableParams.g_OutWidth, ImmutableParams.g_OutHeight,
				ImmutableParams.g_DtxAddressOffset, ImmutableParams.g_DtyAddressOffset);

			GenGradientFoldingPS->SetParameters(RHICmdList, UniformBuffer, DisplacementRenderTarget->TextureRHI);

			DrawPrimitiveUP(RHICmdList, PT_TriangleStrip, 2, PerFrameParams.m_pQuadVB, sizeof(PerFrameParams.m_pQuadVB[0]));
			//RHICopyToResolveTarget(TextureRenderTarget->GetRenderTargetTexture(), TextureRenderTarget->TextureRHI, false, FResolveParams());

			GenGradientFoldingPS->UnsetParameters(RHICmdList);
	});
	
}

//////////////////////////////////////////////////////////////////////////
// Spectrum configuration

const FSpectrumData& AVaOceanSimulator::GetSpectrumConfig() const
{
	return SpectrumConfig;
}

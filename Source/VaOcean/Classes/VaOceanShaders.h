// Copyright 2014-2018 Vladimir Alyamkin. All Rights Reserved.

#pragma once

#include "VaOceanShaders.generated.h"

#define HALF_SQRT_2	0.7071068f
#define GRAV_ACCEL	981.0f	// The acceleration of gravity, cm/s^2

#define BLOCK_SIZE_X 16
#define BLOCK_SIZE_Y 16


//////////////////////////////////////////////////////////////////////////
// Simple Quad vertex shader

/** The vertex data used to render displacement */
struct FQuadVertex
{
	FVector4 Position;
	FVector2D UV;
};

/** The displacement vertex declaration resource type */
class FQuadVertexDeclaration : public FRenderResource
{
public:
	FVertexDeclarationRHIRef VertexDeclarationRHI;

	/** Destructor */
	virtual ~FQuadVertexDeclaration() {}

	virtual void InitRHI()
	{
		FVertexDeclarationElementList Elements;
		Elements.Add(FVertexElement(0, STRUCT_OFFSET(FQuadVertex, Position), VET_Float4, 0, sizeof(FQuadVertex)));
		Elements.Add(FVertexElement(0, STRUCT_OFFSET(FQuadVertex, UV), VET_Float2, 1, sizeof(FQuadVertex)));
		VertexDeclarationRHI = RHICreateVertexDeclaration(Elements);
	}

	virtual void ReleaseRHI()
	{
		VertexDeclarationRHI.SafeRelease();
	}
};

class FQuadVS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FQuadVS, Global);

public:
	static bool ShouldCache(EShaderPlatform Platform)
	{
		return true;
	}

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& PermutationParams)
	{
		// Useful when adding a permutation of a particular shader
		return true;
	}

	FQuadVS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
	}

	FQuadVS() {}
};


//////////////////////////////////////////////////////////////////////////
// Post-FFT data wrap up: Dx, Dy, Dz -> Displacement

BEGIN_UNIFORM_BUFFER_STRUCT(FUpdateDisplacementUniformParameters, )
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(float, ChoppyScale)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(float, GridLen)
END_UNIFORM_BUFFER_STRUCT(FUpdateDisplacementUniformParameters)

typedef TUniformBufferRef<FUpdateDisplacementUniformParameters> FUpdateDisplacementUniformBufferRef;

/** Per frame parameters for UpdateDisplacementPS shader */
USTRUCT()
struct FUpdateDisplacementPSPerFrame
{
	GENERATED_USTRUCT_BODY()

	FVector4 m_pQuadVB[4];

	FShaderResourceViewRHIRef g_InputDxyz;

	// Used to pass params into render thread
	float g_ChoppyScale;
	float g_GridLen;
};

/**
 * Post-FFT data wrap up: Dx, Dy, Dz -> Displacement
 */
class FUpdateDisplacementPS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FUpdateDisplacementPS, Global)

public:
	static bool ShouldCache(EShaderPlatform Platform)
	{
		return IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM5);
	}

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& PermutationParams)
	{
		return true;
	}

	FUpdateDisplacementPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
		ActualDim.Bind(Initializer.ParameterMap, TEXT("g_ActualDim"));
		InWidth.Bind(Initializer.ParameterMap, TEXT("g_InWidth"));
		OutWidth.Bind(Initializer.ParameterMap, TEXT("g_OutWidth"));
		OutHeight.Bind(Initializer.ParameterMap, TEXT("g_OutHeight"));
		DtxAddressOffset.Bind(Initializer.ParameterMap, TEXT("g_DtxAddressOffset"));
		DtyAddressOffset.Bind(Initializer.ParameterMap, TEXT("g_DtyAddressOffset"));

		InputDxyz.Bind(Initializer.ParameterMap, TEXT("g_InputDxyz"));
	}

	FUpdateDisplacementPS()
	{
	}

	void SetParameters(
		FRHICommandList& RHICmdList,
		uint32 ParamActualDim,
		uint32 ParamInWidth,
		uint32 ParamOutWidth,
		uint32 ParamOutHeight,
		uint32 ParamDtxAddressOffset,
		uint32 ParamDtyAddressOffset
		)
	{
		FPixelShaderRHIParamRef PixelShaderRHI = GetPixelShader();

		SetShaderValue(RHICmdList, PixelShaderRHI, ActualDim, ParamActualDim);
		SetShaderValue(RHICmdList, PixelShaderRHI, InWidth, ParamInWidth);
		SetShaderValue(RHICmdList, PixelShaderRHI, OutWidth, ParamOutWidth);
		SetShaderValue(RHICmdList, PixelShaderRHI, OutHeight, ParamOutHeight);
		SetShaderValue(RHICmdList, PixelShaderRHI, DtxAddressOffset, ParamDtxAddressOffset);
		SetShaderValue(RHICmdList, PixelShaderRHI, DtyAddressOffset, ParamDtyAddressOffset);
	}

	void SetParameters(
		FRHICommandList& RHICmdList,
		const FUpdateDisplacementUniformBufferRef& UniformBuffer,
		FShaderResourceViewRHIRef ParamInputDxyz
		)
	{
		FPixelShaderRHIParamRef PixelShaderRHI = GetPixelShader();

		SetUniformBufferParameter(RHICmdList, PixelShaderRHI, GetUniformBufferParameter<FUpdateDisplacementUniformParameters>(), UniformBuffer);

		RHICmdList.SetShaderResourceViewParameter(PixelShaderRHI, InputDxyz.GetBaseIndex(), ParamInputDxyz);
	}

	void UnsetParameters(FRHICommandList& RHICmdList)
	{
		FPixelShaderRHIParamRef PixelShaderRHI = GetPixelShader();

		RHICmdList.SetShaderResourceViewParameter(PixelShaderRHI, InputDxyz.GetBaseIndex(), FShaderResourceViewRHIParamRef());
	}

	virtual bool Serialize(FArchive& Ar)
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << ActualDim << InWidth << OutWidth << OutHeight << DtxAddressOffset << DtyAddressOffset << InputDxyz;

		return bShaderHasOutdatedParameters;
	}

private:
	// Immutable
	FShaderParameter ActualDim;
	FShaderParameter InWidth;
	FShaderParameter OutWidth;
	FShaderParameter OutHeight;
	FShaderParameter DtxAddressOffset;
	FShaderParameter DtyAddressOffset;

	// Buffers
	FShaderResourceParameter InputDxyz;

};


//////////////////////////////////////////////////////////////////////////
// Generate Normal

/** Per frame parameters for UpdateDisplacementPS shader */
USTRUCT()
struct FGenGradientFoldingPSPerFrame
{
	GENERATED_USTRUCT_BODY()

	FVector4 m_pQuadVB[4];

	// Used to pass params into render thread
	float g_ChoppyScale;
	float g_GridLen;
};

/**
 * Displacement -> Normal, Folding
 */
class FGenGradientFoldingPS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FGenGradientFoldingPS, Global)

public:
	static bool ShouldCache(EShaderPlatform Platform)
	{
		return IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM5);
	}

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& PermutationParams)
	{
		return true;
	}

	FGenGradientFoldingPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
		ActualDim.Bind(Initializer.ParameterMap, TEXT("g_ActualDim"));
		InWidth.Bind(Initializer.ParameterMap, TEXT("g_InWidth"));
		OutWidth.Bind(Initializer.ParameterMap, TEXT("g_OutWidth"));
		OutHeight.Bind(Initializer.ParameterMap, TEXT("g_OutHeight"));
		DtxAddressOffset.Bind(Initializer.ParameterMap, TEXT("g_DtxAddressOffset"));
		DtyAddressOffset.Bind(Initializer.ParameterMap, TEXT("g_DtyAddressOffset"));

		DisplacementMap.Bind(Initializer.ParameterMap, TEXT("DisplacementMap"));
		DisplacementMapSampler.Bind(Initializer.ParameterMap, TEXT("DisplacementMapSampler"));
	}

	FGenGradientFoldingPS()
	{
	}

	void SetParameters(
		FRHICommandList& RHICmdList,
		uint32 ParamActualDim,
		uint32 ParamInWidth,
		uint32 ParamOutWidth,
		uint32 ParamOutHeight,
		uint32 ParamDtxAddressOffset,
		uint32 ParamDtyAddressOffset
		)
	{
		FPixelShaderRHIParamRef PixelShaderRHI = GetPixelShader();

		SetShaderValue(RHICmdList, PixelShaderRHI, ActualDim, ParamActualDim);
		SetShaderValue(RHICmdList, PixelShaderRHI, InWidth, ParamInWidth);
		SetShaderValue(RHICmdList, PixelShaderRHI, OutWidth, ParamOutWidth);
		SetShaderValue(RHICmdList, PixelShaderRHI, OutHeight, ParamOutHeight);
		SetShaderValue(RHICmdList, PixelShaderRHI, DtxAddressOffset, ParamDtxAddressOffset);
		SetShaderValue(RHICmdList, PixelShaderRHI, DtyAddressOffset, ParamDtyAddressOffset);
	}

	void SetParameters(
		FRHICommandList& RHICmdList,
		const FUpdateDisplacementUniformBufferRef& UniformBuffer,
		FTextureRHIParamRef DisplacementMapRHI
		)
	{
		FPixelShaderRHIParamRef PixelShaderRHI = GetPixelShader();

		SetUniformBufferParameter(RHICmdList, PixelShaderRHI, GetUniformBufferParameter<FUpdateDisplacementUniformParameters>(), UniformBuffer);

		FSamplerStateRHIParamRef SamplerStateLinear = TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();
		SetTextureParameter(RHICmdList, PixelShaderRHI, DisplacementMap, DisplacementMapSampler, SamplerStateLinear, DisplacementMapRHI);
	}

	void UnsetParameters(FRHICommandList& RHICmdList) {}

	virtual bool Serialize(FArchive& Ar)
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << ActualDim << InWidth << OutWidth << OutHeight << DtxAddressOffset << DtyAddressOffset
			<< DisplacementMap << DisplacementMapSampler;

		return bShaderHasOutdatedParameters;
	}

private:
	// Immutable
	FShaderParameter ActualDim;
	FShaderParameter InWidth;
	FShaderParameter OutWidth;
	FShaderParameter OutHeight;
	FShaderParameter DtxAddressOffset;
	FShaderParameter DtyAddressOffset;

	// Displacement map
	FShaderResourceParameter DisplacementMap;
	FShaderResourceParameter DisplacementMapSampler;

};


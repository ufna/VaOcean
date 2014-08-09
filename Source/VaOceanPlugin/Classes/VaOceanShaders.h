// Copyright 2014 Vladimir Alyamkin. All Rights Reserved.

#pragma once

#include "VaOceanPluginPrivatePCH.h"
#include "VaOceanShaders.generated.h"

#define HALF_SQRT_2	0.7071068f
#define GRAV_ACCEL	981.0f	// The acceleration of gravity, cm/s^2

#define BLOCK_SIZE_X 16
#define BLOCK_SIZE_Y 16


//////////////////////////////////////////////////////////////////////////
// UpdateSpectrumCS compute shader

BEGIN_UNIFORM_BUFFER_STRUCT(FUpdateSpectrumUniformParameters, )
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(float, Time)
END_UNIFORM_BUFFER_STRUCT(FUpdateSpectrumUniformParameters)

typedef TUniformBufferRef<FUpdateSpectrumUniformParameters> FUpdateSpectrumUniformBufferRef;

/** Immutable parameters for UpdateSpectrumCS shader*/
USTRUCT()
struct FUpdateSpectrumCSImmutable
{
	GENERATED_USTRUCT_BODY()

	uint32 g_ActualDim;
	uint32 g_InWidth;
	uint32 g_OutWidth;
	uint32 g_OutHeight;
	uint32 g_DtxAddressOffset;
	uint32 g_DtyAddressOffset;
};

/** Per frame parameters for UpdateSpectrumCS shader */
USTRUCT()
struct FUpdateSpectrumCSPerFrame
{
	GENERATED_USTRUCT_BODY()

	FShaderResourceViewRHIRef m_pSRV_H0;
	FShaderResourceViewRHIRef m_pSRV_Omega;
	FUnorderedAccessViewRHIRef m_pUAV_Ht;

	// Used to pass params into render thread
	float g_Time;
	float g_ChoppyScale;
};

/**
 * H(0) -> H(t), D(x,t), D(y,t)
 */
class FUpdateSpectrumCS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FUpdateSpectrumCS, Global)

public:
	static bool ShouldCache(EShaderPlatform Platform)
	{
		return IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM5);
	}

	FUpdateSpectrumCS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
		ActualDim.Bind(Initializer.ParameterMap, TEXT("g_ActualDim"), SPF_Mandatory);
		InWidth.Bind(Initializer.ParameterMap, TEXT("g_InWidth"), SPF_Mandatory);
		OutWidth.Bind(Initializer.ParameterMap, TEXT("g_OutWidth"), SPF_Mandatory);
		OutHeight.Bind(Initializer.ParameterMap, TEXT("g_OutHeight"), SPF_Mandatory);
		DtxAddressOffset.Bind(Initializer.ParameterMap, TEXT("g_DtxAddressOffset"), SPF_Mandatory);
		DtyAddressOffset.Bind(Initializer.ParameterMap, TEXT("g_DtyAddressOffset"), SPF_Mandatory);

		InputH0.Bind(Initializer.ParameterMap, TEXT("g_InputH0"), SPF_Mandatory);
		InputOmega.Bind(Initializer.ParameterMap, TEXT("g_InputOmega"), SPF_Mandatory);

		OutputHtRW.Bind(Initializer.ParameterMap, TEXT("g_OutputHt"), SPF_Mandatory);
	}

	FUpdateSpectrumCS()
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
		FComputeShaderRHIParamRef ComputeShaderRHI = GetComputeShader();

		SetShaderValue(RHICmdList, ComputeShaderRHI, ActualDim, ParamActualDim);
		SetShaderValue(RHICmdList, ComputeShaderRHI, InWidth, ParamInWidth);
		SetShaderValue(RHICmdList, ComputeShaderRHI, OutWidth, ParamOutWidth);
		SetShaderValue(RHICmdList, ComputeShaderRHI, OutHeight, ParamOutHeight);
		SetShaderValue(RHICmdList, ComputeShaderRHI, DtxAddressOffset, ParamDtxAddressOffset);
		SetShaderValue(RHICmdList, ComputeShaderRHI, DtyAddressOffset, ParamDtyAddressOffset);
	}

	void SetParameters(
		FRHICommandList& RHICmdList,
		const FUpdateSpectrumUniformBufferRef& UniformBuffer,
		FShaderResourceViewRHIRef ParamInputH0,
		FShaderResourceViewRHIRef ParamInputOmega
		)
	{
		FComputeShaderRHIParamRef ComputeShaderRHI = GetComputeShader();

		SetUniformBufferParameter(RHICmdList, ComputeShaderRHI, GetUniformBufferParameter<FUpdateSpectrumUniformParameters>(), UniformBuffer);

		RHICmdList.SetShaderResourceViewParameter(ComputeShaderRHI, InputH0.GetBaseIndex(), ParamInputH0);
		RHICmdList.SetShaderResourceViewParameter(ComputeShaderRHI, InputOmega.GetBaseIndex(), ParamInputOmega);
	}

	void UnsetParameters(FRHICommandList &RHICmdList)
	{
		FComputeShaderRHIParamRef ComputeShaderRHI = GetComputeShader();
		FShaderResourceViewRHIParamRef NullSRV = FShaderResourceViewRHIParamRef();

		RHICmdList.SetShaderResourceViewParameter(ComputeShaderRHI, InputH0.GetBaseIndex(), NullSRV);
		RHICmdList.SetShaderResourceViewParameter(ComputeShaderRHI, InputOmega.GetBaseIndex(), NullSRV);
	}

	void SetOutput(FRHICommandList& RHICmdList, FUnorderedAccessViewRHIParamRef ParamOutputHtRW)
	{
		FComputeShaderRHIParamRef ComputeShaderRHI = GetComputeShader();
		if (OutputHtRW.IsBound())
		{
			RHICmdList.SetUAVParameter(ComputeShaderRHI, OutputHtRW.GetBaseIndex(), ParamOutputHtRW);
		}
	}

	void UnbindBuffers(FRHICommandList& RHICmdList)
	{
		FComputeShaderRHIParamRef ComputeShaderRHI = GetComputeShader();
		if (OutputHtRW.IsBound())
		{
			RHICmdList.SetUAVParameter(ComputeShaderRHI, OutputHtRW.GetBaseIndex(), FUnorderedAccessViewRHIParamRef());
		}
	}

	virtual bool Serialize(FArchive& Ar)
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << ActualDim << InWidth << OutWidth << OutHeight << DtxAddressOffset << DtyAddressOffset
			<< InputH0 << InputOmega << OutputHtRW;

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
	FShaderResourceParameter InputH0;
	FShaderResourceParameter InputOmega;
	FShaderResourceParameter OutputHtRW;

};


//////////////////////////////////////////////////////////////////////////
// Radix008A_CS compute shader

BEGIN_UNIFORM_BUFFER_STRUCT(FRadixFFTUniformParameters, )
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(uint32, ThreadCount)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(uint32, ostride)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(uint32, istride)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(uint32, pstride)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(float, PhaseBase)
END_UNIFORM_BUFFER_STRUCT(FRadixFFTUniformParameters)

typedef TUniformBufferRef<FRadixFFTUniformParameters> FRadixFFTUniformBufferRef;

/**
 * FFT calculations for istride > 1 
 */
class FRadix008A_CS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FRadix008A_CS, Global)

public:
	static bool ShouldCache(EShaderPlatform Platform)
	{
		return IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM5);
	}

	FRadix008A_CS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
		SrcData.Bind(Initializer.ParameterMap, TEXT("g_SrcData"));
		DstData.Bind(Initializer.ParameterMap, TEXT("g_DstData"));
	}

	FRadix008A_CS()
	{
	}

	void SetParameters(FRHICommandList& RHICmdList, const FRadixFFTUniformBufferRef& UniformBuffer)
	{
		FComputeShaderRHIParamRef ComputeShaderRHI = GetComputeShader();

		SetUniformBufferParameter(RHICmdList, ComputeShaderRHI, GetUniformBufferParameter<FRadixFFTUniformParameters>(), UniformBuffer);
	}

	void SetParameters(FRHICommandList& RHICmdList, FShaderResourceViewRHIRef ParamSrcData, FUnorderedAccessViewRHIRef ParamDstData)
	{
		FComputeShaderRHIParamRef ComputeShaderRHI = GetComputeShader();

		RHICmdList.SetShaderResourceViewParameter(ComputeShaderRHI, SrcData.GetBaseIndex(), ParamSrcData);
		RHICmdList.SetUAVParameter(ComputeShaderRHI, DstData.GetBaseIndex(), ParamDstData);
	}

	void UnsetParameters(FRHICommandList& RHICmdList)
	{
		FComputeShaderRHIParamRef ComputeShaderRHI = GetComputeShader();

		RHICmdList.SetShaderResourceViewParameter(ComputeShaderRHI, SrcData.GetBaseIndex(), FShaderResourceViewRHIParamRef());
		RHICmdList.SetUAVParameter(ComputeShaderRHI, DstData.GetBaseIndex(), FUnorderedAccessViewRHIParamRef());
	}

	virtual bool Serialize(FArchive& Ar)
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << SrcData << DstData;

		return bShaderHasOutdatedParameters;
	}

private:
	// Buffers
	FShaderResourceParameter SrcData;
	FShaderResourceParameter DstData;

};

/**
 * pstride and istride parameters are excess here, but we use inheritance as its easier for now
 */
class FRadix008A_CS2 : public FRadix008A_CS
{
	DECLARE_SHADER_TYPE(FRadix008A_CS2, Global)

public:
	FRadix008A_CS2(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FRadix008A_CS(Initializer)
	{
	}

	FRadix008A_CS2()
	{
	}
};


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


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

	float g_Time;
	float g_ChoppyScale;

	FShaderResourceViewRHIRef m_pSRV_H0;
	FShaderResourceViewRHIRef m_pSRV_Omega;
	FUnorderedAccessViewRHIRef m_pUAV_Ht;
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

		Time.Bind(Initializer.ParameterMap, TEXT("g_Time"), SPF_Mandatory);
		ChoppyScale.Bind(Initializer.ParameterMap, TEXT("g_ChoppyScale"));

		InputH0.Bind(Initializer.ParameterMap, TEXT("g_InputH0"), SPF_Mandatory);
		InputOmega.Bind(Initializer.ParameterMap, TEXT("g_InputOmega"), SPF_Mandatory);
		OutputHtRW.Bind(Initializer.ParameterMap, TEXT("g_OutputHt"), SPF_Mandatory);
	}

	FUpdateSpectrumCS()
	{
	}

	void SetParameters(
		uint32 ParamActualDim,
		uint32 ParamInWidth,
		uint32 ParamOutWidth,
		uint32 ParamOutHeight,
		uint32 ParamDtxAddressOffset,
		uint32 ParamDtyAddressOffset
		)
	{
		FComputeShaderRHIParamRef ComputeShaderRHI = GetComputeShader();

		SetShaderValue(ComputeShaderRHI, ActualDim, ParamActualDim);
		SetShaderValue(ComputeShaderRHI, InWidth, ParamInWidth);
		SetShaderValue(ComputeShaderRHI, OutWidth, ParamOutWidth);
		SetShaderValue(ComputeShaderRHI, OutHeight, ParamOutHeight);
		SetShaderValue(ComputeShaderRHI, DtxAddressOffset, ParamDtxAddressOffset);
		SetShaderValue(ComputeShaderRHI, DtyAddressOffset, ParamDtyAddressOffset);
	}

	void SetParameters(
		uint32 ParamTime,
		uint32 ParamChoppyScale,
		FShaderResourceViewRHIRef ParamInputH0,
		FShaderResourceViewRHIRef ParamInputOmega,
		FUnorderedAccessViewRHIRef ParamOutputHtRW
		)
	{
		FComputeShaderRHIParamRef ComputeShaderRHI = GetComputeShader();

		SetShaderValue(ComputeShaderRHI, Time, ParamTime);
		SetShaderValue(ComputeShaderRHI, ChoppyScale, ParamChoppyScale);

		RHISetShaderResourceViewParameter(ComputeShaderRHI, InputH0.GetBaseIndex(), ParamInputH0);
		RHISetShaderResourceViewParameter(ComputeShaderRHI, InputOmega.GetBaseIndex(), ParamInputOmega);

		RHISetUAVParameter(ComputeShaderRHI, OutputHtRW.GetBaseIndex(), ParamOutputHtRW);
	}

	void UnsetParameters()
	{
		FComputeShaderRHIParamRef ComputeShaderRHI = GetComputeShader();
		FShaderResourceViewRHIParamRef NullSRV = FShaderResourceViewRHIParamRef();

		RHISetShaderResourceViewParameter(ComputeShaderRHI, InputH0.GetBaseIndex(), NullSRV);
		RHISetShaderResourceViewParameter(ComputeShaderRHI, InputOmega.GetBaseIndex(), NullSRV);
		RHISetUAVParameter(ComputeShaderRHI, OutputHtRW.GetBaseIndex(), FUnorderedAccessViewRHIParamRef());
	}

	virtual bool Serialize(FArchive& Ar)
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << ActualDim << InWidth << OutWidth << OutHeight << DtxAddressOffset << DtyAddressOffset
			<< Time << ChoppyScale << InputH0 << InputOmega << OutputHtRW;

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

	// Changed per frame
	FShaderParameter Time;
	FShaderParameter ChoppyScale;

	// Buffers
	FShaderResourceParameter InputH0;
	FShaderResourceParameter InputOmega;
	FShaderResourceParameter OutputHtRW;

};


//////////////////////////////////////////////////////////////////////////
// Radix008A_CS compute shader

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
		ThreadCount.Bind(Initializer.ParameterMap, TEXT("g_ThreadCount"));
		ostride.Bind(Initializer.ParameterMap, TEXT("g_ostride"));
		istride.Bind(Initializer.ParameterMap, TEXT("g_istride"));
		pstride.Bind(Initializer.ParameterMap, TEXT("g_pstride"));
		PhaseBase.Bind(Initializer.ParameterMap, TEXT("g_PhaseBase"));

		SrcData.Bind(Initializer.ParameterMap, TEXT("g_SrcData"));
		DstData.Bind(Initializer.ParameterMap, TEXT("g_DstData"));
	}

	FRadix008A_CS()
	{
	}

	void SetParameters(
		uint32 ParamThreadCount,
		uint32 Param_ostride,
		uint32 Param_istride,
		uint32 Param_pstride,
		uint32 ParamPhaseBase
		)
	{
		FComputeShaderRHIParamRef ComputeShaderRHI = GetComputeShader();

		SetShaderValue(ComputeShaderRHI, ThreadCount, ParamThreadCount);
		SetShaderValue(ComputeShaderRHI, ostride, Param_ostride);
		SetShaderValue(ComputeShaderRHI, istride, Param_istride);
		SetShaderValue(ComputeShaderRHI, pstride, Param_pstride);
		SetShaderValue(ComputeShaderRHI, PhaseBase, ParamPhaseBase);
	}

	void SetParameters(FShaderResourceViewRHIRef ParamSrcData, FUnorderedAccessViewRHIRef ParamDstData)
	{
		FComputeShaderRHIParamRef ComputeShaderRHI = GetComputeShader();

		RHISetShaderResourceViewParameter(ComputeShaderRHI, SrcData.GetBaseIndex(), ParamSrcData);
		RHISetUAVParameter(ComputeShaderRHI, DstData.GetBaseIndex(), ParamDstData);
	}

	void UnsetParameters()
	{
		FComputeShaderRHIParamRef ComputeShaderRHI = GetComputeShader();

		RHISetShaderResourceViewParameter(ComputeShaderRHI, SrcData.GetBaseIndex(), FShaderResourceViewRHIParamRef());
		RHISetUAVParameter(ComputeShaderRHI, DstData.GetBaseIndex(), FUnorderedAccessViewRHIParamRef());
	}

	virtual bool Serialize(FArchive& Ar)
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << ThreadCount << ostride << istride << pstride << PhaseBase
			<< SrcData << DstData;

		return bShaderHasOutdatedParameters;
	}

private:
	// Changed per call
	FShaderParameter ThreadCount;
	FShaderParameter ostride;
	FShaderParameter istride;
	FShaderParameter pstride;
	FShaderParameter PhaseBase;

	// Buffers
	FShaderResourceParameter SrcData;
	FShaderResourceParameter DstData;

};

/**
 * FFT calculations for istride == 1
 */
class FRadix008A_CS2 : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FRadix008A_CS2, Global)

public:
	static bool ShouldCache(EShaderPlatform Platform)
	{
		return IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM5);
	}

	FRadix008A_CS2(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
		ThreadCount.Bind(Initializer.ParameterMap, TEXT("g_ThreadCount"));
		ostride.Bind(Initializer.ParameterMap, TEXT("g_ostride"));
		istride.Bind(Initializer.ParameterMap, TEXT("g_istride"));
		pstride.Bind(Initializer.ParameterMap, TEXT("g_pstride"));
		PhaseBase.Bind(Initializer.ParameterMap, TEXT("g_PhaseBase"));

		SrcData.Bind(Initializer.ParameterMap, TEXT("g_SrcData"));
		DstData.Bind(Initializer.ParameterMap, TEXT("g_DstData"));
	}

	FRadix008A_CS2()
	{
	}

	void SetParameters(
		uint32 ParamThreadCount,
		uint32 Param_ostride,
		uint32 Param_istride,
		uint32 Param_pstride,
		uint32 ParamPhaseBase
		)
	{
		FComputeShaderRHIParamRef ComputeShaderRHI = GetComputeShader();

		SetShaderValue(ComputeShaderRHI, ThreadCount, ParamThreadCount);
		SetShaderValue(ComputeShaderRHI, ostride, Param_ostride);
		SetShaderValue(ComputeShaderRHI, istride, Param_istride);
		SetShaderValue(ComputeShaderRHI, pstride, Param_pstride);
		SetShaderValue(ComputeShaderRHI, PhaseBase, ParamPhaseBase);
	}

	void SetParameters(FShaderResourceViewRHIRef ParamSrcData, FUnorderedAccessViewRHIRef ParamDstData)
	{
		FComputeShaderRHIParamRef ComputeShaderRHI = GetComputeShader();

		RHISetShaderResourceViewParameter(ComputeShaderRHI, SrcData.GetBaseIndex(), ParamSrcData);
		RHISetUAVParameter(ComputeShaderRHI, DstData.GetBaseIndex(), ParamDstData);
	}

	void UnsetParameters()
	{
		FComputeShaderRHIParamRef ComputeShaderRHI = GetComputeShader();

		RHISetShaderResourceViewParameter(ComputeShaderRHI, SrcData.GetBaseIndex(), FShaderResourceViewRHIParamRef());
		RHISetUAVParameter(ComputeShaderRHI, DstData.GetBaseIndex(), FUnorderedAccessViewRHIParamRef());
	}

	virtual bool Serialize(FArchive& Ar)
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << ThreadCount << ostride << istride << pstride << PhaseBase
			<< SrcData << DstData;

		return bShaderHasOutdatedParameters;
	}

private:
	// Changed per call
	FShaderParameter ThreadCount;
	FShaderParameter ostride;
	FShaderParameter istride;
	FShaderParameter pstride;
	FShaderParameter PhaseBase;

	// Buffers
	FShaderResourceParameter SrcData;
	FShaderResourceParameter DstData;

};

/**
 * pstride and istride parameters are excess here, but we use inheritance as its easier for now
 * /
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
};*/


//////////////////////////////////////////////////////////////////////////
// Post-FFT data wrap up: Dx, Dy, Dz -> Displacement

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
		Elements.Add(FVertexElement(0, STRUCT_OFFSET(FQuadVertex, Position), VET_Float4, 0));
		Elements.Add(FVertexElement(0, STRUCT_OFFSET(FQuadVertex, UV), VET_Float2, 1));
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

/** Per frame parameters for UpdateDisplacementPS shader */
USTRUCT()
struct FUpdateDisplacementPSPerFrame
{
	GENERATED_USTRUCT_BODY()

	float g_Time;
	float g_ChoppyScale;
	float g_GridLen;

	FVector4 m_pQuadVB[4];

	FShaderResourceViewRHIRef g_InputDxyz;
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

		Time.Bind(Initializer.ParameterMap, TEXT("g_Time"));
		ChoppyScale.Bind(Initializer.ParameterMap, TEXT("g_ChoppyScale"));
		GridLen.Bind(Initializer.ParameterMap, TEXT("g_GridLen"));

		InputDxyz.Bind(Initializer.ParameterMap, TEXT("g_InputDxyz"));
	}

	FUpdateDisplacementPS()
	{
	}

	void SetParameters(
		uint32 ParamActualDim,
		uint32 ParamInWidth,
		uint32 ParamOutWidth,
		uint32 ParamOutHeight,
		uint32 ParamDtxAddressOffset,
		uint32 ParamDtyAddressOffset
		)
	{
		FPixelShaderRHIParamRef PixelShaderRHI = GetPixelShader();

		SetShaderValue(PixelShaderRHI, ActualDim, ParamActualDim);
		SetShaderValue(PixelShaderRHI, InWidth, ParamInWidth);
		SetShaderValue(PixelShaderRHI, OutWidth, ParamOutWidth);
		SetShaderValue(PixelShaderRHI, OutHeight, ParamOutHeight);
		SetShaderValue(PixelShaderRHI, DtxAddressOffset, ParamDtxAddressOffset);
		SetShaderValue(PixelShaderRHI, DtyAddressOffset, ParamDtyAddressOffset);
	}

	void SetParameters(
		uint32 ParamTime,
		uint32 ParamChoppyScale,
		uint32 ParamGridLen,
		FShaderResourceViewRHIRef ParamInputDxyz
		)
	{
		FPixelShaderRHIParamRef PixelShaderRHI = GetPixelShader();

		SetShaderValue(PixelShaderRHI, Time, ParamTime);
		SetShaderValue(PixelShaderRHI, ChoppyScale, ParamChoppyScale);
		SetShaderValue(PixelShaderRHI, GridLen, ParamGridLen);

		RHISetShaderResourceViewParameter(PixelShaderRHI, InputDxyz.GetBaseIndex(), ParamInputDxyz);
	}

	void UnsetParameters()
	{
		FPixelShaderRHIParamRef PixelShaderRHI = GetPixelShader();

		RHISetShaderResourceViewParameter(PixelShaderRHI, InputDxyz.GetBaseIndex(), FShaderResourceViewRHIParamRef());
	}

	virtual bool Serialize(FArchive& Ar)
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << ActualDim << InWidth << OutWidth << OutHeight << DtxAddressOffset << DtyAddressOffset
			<< Time << ChoppyScale << GridLen << InputDxyz;

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

	// Changed per frame
	FShaderParameter Time;
	FShaderParameter ChoppyScale;
	FShaderParameter GridLen;

	// Buffers
	FShaderResourceParameter InputDxyz;

};

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
		ActualDim.Bind(Initializer.ParameterMap, TEXT("g_ActualDim"));
		InWidth.Bind(Initializer.ParameterMap, TEXT("g_InWidth"));
		OutWidth.Bind(Initializer.ParameterMap, TEXT("g_OutWidth"));
		OutHeight.Bind(Initializer.ParameterMap, TEXT("g_OutHeight"));
		DtxAddressOffset.Bind(Initializer.ParameterMap, TEXT("g_DtxAddressOffset"));
		DtyAddressOffset.Bind(Initializer.ParameterMap, TEXT("g_DtyAddressOffset"));

		Time.Bind(Initializer.ParameterMap, TEXT("g_Time"));
		ChoppyScale.Bind(Initializer.ParameterMap, TEXT("g_ChoppyScale"));

		InputH0.Bind(Initializer.ParameterMap, TEXT("g_InputH0"));
		InputOmega.Bind(Initializer.ParameterMap, TEXT("g_InputOmega"));
		OutputHtRW.Bind(Initializer.ParameterMap, TEXT("g_OutputHt"));
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

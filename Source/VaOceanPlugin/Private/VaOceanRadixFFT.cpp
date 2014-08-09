// Copyright 2014 Vladimir Alyamkin. All Rights Reserved.

#include "VaOceanPluginPrivatePCH.h"

void Radix008A(
	FRHICommandListImmediate & RHICmdList,
	FRadixPlan512* Plan,
	uint32 ParamSet,
	FUnorderedAccessViewRHIRef pUAV_Dst,
	FShaderResourceViewRHIRef pSRV_Src,
	uint32 ThreadCount,
	uint32 istride)
{
	check(ParamSet < FFT_PARAM_SETS);

	// Setup execution configuration
	uint32 grid = ThreadCount / COHERENCY_GRANULARITY;

	FRadixFFTUniformParameters Parameters;
	Parameters.ThreadCount = Plan->PerFrame[ParamSet].ThreadCount;
	Parameters.ostride = Plan->PerFrame[ParamSet].ostride;
	Parameters.istride = Plan->PerFrame[ParamSet].istride;
	Parameters.pstride = Plan->PerFrame[ParamSet].pstride;
	Parameters.PhaseBase = Plan->PerFrame[ParamSet].PhaseBase;

	FRadixFFTUniformBufferRef UniformBuffer =
		FRadixFFTUniformBufferRef::CreateUniformBufferImmediate(Parameters, EUniformBufferUsage::UniformBuffer_SingleFrame);

	if (istride > 1)
	{
		TShaderMapRef<FRadix008A_CS> Radix008A_CS(GetGlobalShaderMap());
		RHICmdList.SetComputeShader(Radix008A_CS->GetComputeShader());

		Radix008A_CS->SetParameters(RHICmdList, UniformBuffer);
		Radix008A_CS->SetParameters(RHICmdList, pSRV_Src, pUAV_Dst);

		RHICmdList.DispatchComputeShader(grid, 1, 1);

		Radix008A_CS->UnsetParameters(RHICmdList);
	}
	else
	{
		TShaderMapRef<FRadix008A_CS2> Radix008A_CS2(GetGlobalShaderMap());
		RHICmdList.SetComputeShader(Radix008A_CS2->GetComputeShader());

		Radix008A_CS2->SetParameters(RHICmdList, UniformBuffer);
		Radix008A_CS2->SetParameters(RHICmdList, pSRV_Src, pUAV_Dst);

		RHICmdList.DispatchComputeShader(grid, 1, 1);

		Radix008A_CS2->UnsetParameters(RHICmdList);
	}
}

void RadixSetPerFrameParams(FRadixPlan512* Plan,
	uint32 ParamSet,
	uint32 ThreadCount,
	uint32 ostride,
	uint32 istride,
	uint32 pstride,
	float PhaseBase)
{
	check(ParamSet < FFT_PARAM_SETS);

	Plan->PerFrame[ParamSet].ThreadCount = ThreadCount;
	Plan->PerFrame[ParamSet].ostride = ostride;
	Plan->PerFrame[ParamSet].istride = istride;
	Plan->PerFrame[ParamSet].pstride = pstride;
	Plan->PerFrame[ParamSet].PhaseBase = PhaseBase;
}

void RadixCreatePlan(FRadixPlan512* Plan, uint32 Slices)
{
	Plan->Slices = Slices;

	// Create 6 param sets for 512x512 transform
	const uint32 thread_count = Plan->Slices * (512 * 512) / 8;
	uint32 ostride = 512 * 512 / 8;
	uint32 istride = ostride;
	uint32 pstride = 512;
	double phase_base = -TWO_PI / (512.0 * 512.0);

	RadixSetPerFrameParams(Plan, 0, thread_count, ostride, istride, pstride, (float)phase_base);

	for (int i = 1; i < FFT_PARAM_SETS; i++)
	{
		istride /= 8;
		phase_base *= 8.0;

		if (i == 3)
		{
			ostride /= 512;
			pstride = 1;
		}

		RadixSetPerFrameParams(Plan, i, thread_count, ostride, istride, pstride, (float)phase_base);
	}
	
	// Temp buffers
	uint32 BytesPerElement = sizeof(float) * 2;
	uint32 NumElements = (512 * Plan->Slices) * 512;

	FRHIResourceCreateInfo ResourceCreateInfo;
	ResourceCreateInfo.BulkData = nullptr;
	ResourceCreateInfo.ResourceArray = nullptr;
	Plan->pBuffer_Tmp = RHICreateStructuredBuffer(BytesPerElement, BytesPerElement * NumElements, (BUF_UnorderedAccess | BUF_ShaderResource), ResourceCreateInfo);

	Plan->pUAV_Tmp = RHICreateUnorderedAccessView(Plan->pBuffer_Tmp, false, false);
	Plan->pSRV_Tmp = RHICreateShaderResourceView(Plan->pBuffer_Tmp);
}

void RadixDestroyPlan(FRadixPlan512* Plan)
{
	Plan->pBuffer_Tmp.SafeRelease();
	Plan->pUAV_Tmp.SafeRelease();
	Plan->pSRV_Tmp.SafeRelease();
}

void RadixCompute(
	FRHICommandListImmediate& RHICmdList,
	FRadixPlan512* Plan,
	FUnorderedAccessViewRHIRef pUAV_Dst,
	FShaderResourceViewRHIRef pSRV_Dst,
	FShaderResourceViewRHIRef pSRV_Src)
{
	const uint32 thread_count = Plan->Slices * (512 * 512) / 8;
	uint32 istride = 512 * 512;

	FUnorderedAccessViewRHIRef pUAV_Tmp = Plan->pUAV_Tmp;
	FShaderResourceViewRHIRef pSRV_Tmp = Plan->pSRV_Tmp;

	istride /= 8;
	Radix008A(RHICmdList, Plan, 0, pUAV_Tmp, pSRV_Src, thread_count, istride);

	istride /= 8;
	Radix008A(RHICmdList, Plan, 1, pUAV_Dst, pSRV_Tmp, thread_count, istride);

	istride /= 8;
	Radix008A(RHICmdList, Plan, 2, pUAV_Tmp, pSRV_Dst, thread_count, istride);

	istride /= 8;
	Radix008A(RHICmdList, Plan, 3, pUAV_Dst, pSRV_Tmp, thread_count, istride);

	istride /= 8;
	Radix008A(RHICmdList, Plan, 4, pUAV_Tmp, pSRV_Dst, thread_count, istride);

	istride /= 8;
	Radix008A(RHICmdList, Plan, 5, pUAV_Dst, pSRV_Tmp, thread_count, istride);
}

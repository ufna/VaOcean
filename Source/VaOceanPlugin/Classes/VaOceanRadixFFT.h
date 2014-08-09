// Copyright 2014 Vladimir Alyamkin. All Rights Reserved.

#pragma once

#include "VaOceanPluginPrivatePCH.h"
#include "VaOceanRadixFFT.generated.h"

/** Memory access coherency (in threads) */
#define COHERENCY_GRANULARITY 128

/** Common constants */
#define TWO_PI 6.283185307179586476925286766559

#define FFT_DIMENSIONS 3U
#define FFT_PLAN_SIZE_LIMIT (1U << 27)

#define FFT_FORWARD -1
#define FFT_INVERSE 1

#define FFT_PARAM_SETS 6

/** Per frame parameters for FRadix008A_CS shader */
USTRUCT()
struct FRadix008A_CSPerFrame
{
	GENERATED_USTRUCT_BODY()

	uint32 ThreadCount;
	uint32 ostride;
	uint32 istride;
	uint32 pstride;
	float PhaseBase;
};

/** Radix FFT data (for 512x512 buffer size) */
USTRUCT()
struct FRadixPlan512
{
	GENERATED_USTRUCT_BODY()

	// More than one array can be transformed at same time
	uint32 Slices;

	// For 512x512 config, we need 6 sets of parameters
	FRadix008A_CSPerFrame PerFrame[FFT_PARAM_SETS];

	// Temporary buffers
	FStructuredBufferRHIRef pBuffer_Tmp;
	FUnorderedAccessViewRHIRef pUAV_Tmp;
	FShaderResourceViewRHIRef pSRV_Tmp;
};


void RadixCreatePlan(FRadixPlan512* Plan, uint32 Slices);
void RadixDestroyPlan(FRadixPlan512* Plan);

void RadixCompute(	FRHICommandListImmediate& RHICmdList,
					FRadixPlan512* Plan,
					FUnorderedAccessViewRHIRef pUAV_Dst,
					FShaderResourceViewRHIRef pSRV_Dst, 
					FShaderResourceViewRHIRef pSRV_Src);

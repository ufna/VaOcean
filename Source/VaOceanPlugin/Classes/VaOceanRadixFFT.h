// Created 2014-2016 Vladimir Alyamkin. MIT license 

//#define LOCTEXT_NAMESPACE "team"
#pragma once

//#include "Runtime/Engine/Classes/Engine/DataTable.h"
#include "VaOceanPluginPrivatePCH.h"
#include "VaOceanTypes.h"
#include "VaStructs.h"
//#include "VaOceanRadixFFT.h"
//#include "VaOceanSimulator.h"
#include "VaOceanRadixFFT.generated.h"

/** Memory access coherency (in threads) */
#define COHERENCY_GRANULARITY 128

/** Common constants */
#define TWO_PI 6.283185307179586476925286766559

#define FFT_DIMENSIONS 3U
#define FFT_PLAN_SIZE_LIMIT (1U << 27)

#define FFT_FORWARD -1
#define FFT_INVERSE 1



USTRUCT()
struct FdummyStruct
{
	GENERATED_BODY()

	UPROPERTY()
	uint32 dummyParm;

};

void RadixCreatePlan(FRadixPlan512* Plan, uint32 Slices);
void RadixDestroyPlan(FRadixPlan512* Plan);

void RadixCompute(FRHICommandListImmediate& RHICmdList,
	FRadixPlan512* Plan,
	FUnorderedAccessViewRHIRef pUAV_Dst,
	FShaderResourceViewRHIRef pSRV_Dst,
	FShaderResourceViewRHIRef pSRV_Src);

//#undef LOCTEXT_NAMESPACE
#pragma once

#include "VaOceanPluginPrivatePCH.h"
#include "VaStructs.generated.h"

#define FFT_PARAM_SETS 6

/** Per frame parameters for FRadix008A_CS shader */
USTRUCT()
struct FRadix008A_CSPerFrame
{
	GENERATED_BODY()

		//GENERATED_BODY()

	//UPROPERTY(EditAnywhere, BlueprintReadWrite)
		uint32 ThreadCount;

	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FBonus Struct")
	uint32 ostride;
	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FBonus Struct")
	uint32 istride;
	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FBonus Struct")
	uint32 pstride;
	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FBonus Struct")
	float PhaseBase;

	//FRadix008A_CSPerFrame::FRadix008A_CSPerFrame()
	//{
	//	ThreadCount = 0;
	//	ostride = 0;
	//	istride = 0;
	//	pstride = 0;
	//	PhaseBase = 0;
	//}
};

/** Radix FFT data (for 512x512 buffer size) */
USTRUCT()
struct FRadixPlan512
{
	GENERATED_BODY()

public:
	// More than one array can be transformed at same time
	UPROPERTY()
		uint32 Slices;

	// For 512x512 config, we need 6 sets of parameters
	//UPROPERTY()
	FRadix008A_CSPerFrame PerFrame[FFT_PARAM_SETS];

	// Temporary buffers
	//UPROPERTY()
	FStructuredBufferRHIRef pBuffer_Tmp;
	//UPROPERTY()
	FUnorderedAccessViewRHIRef pUAV_Tmp;
	//UPROPERTY()
	FShaderResourceViewRHIRef pSRV_Tmp;
};
// Copyright 2014 Vladimir Alyamkin. All Rights Reserved.

#pragma once

#include "VaOceanTypes.generated.h"

/** Phillips spectrum configuration */
USTRUCT(BlueprintType)
struct FSpectrumData
{
	GENERATED_USTRUCT_BODY()

	/** The size of displacement map. Must be power of 2.
	 * Not editable because of FFT shader config */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 DispMapDimension;

	/** The side length (world space) of square patch. Typical value is 1000 ~ 2000. */
	UPROPERTY(EditAnywhere)
	float PatchLength;

	/** Adjust the time interval for simulation (controls the simulation speed) */
	UPROPERTY(EditAnywhere)
	float TimeScale;

	/** Amplitude for transverse wave. Around 1.0 (not the world space height). */
	UPROPERTY(EditAnywhere)
	float WaveAmplitude;

	/** Wind direction. Normalization not required */
	UPROPERTY(EditAnywhere)
	FVector2D WindDirection;

	/** The bigger the wind speed, the larger scale of wave crest. But the wave scale can be no larger than PatchLength. Around 100 ~ 1000 */
	UPROPERTY(EditAnywhere)
	float WindSpeed;

	/** This value damps out the waves against the wind direction. Smaller value means higher wind dependency. */
	UPROPERTY(EditAnywhere)
	float WindDependency;

	/** The amplitude for longitudinal wave. Higher value creates pointy crests. Must be positive. */
	UPROPERTY(EditAnywhere)
	float ChoppyScale;

	/** Defaults */
	FSpectrumData()
	{
		DispMapDimension = 512;
		PatchLength = 2000.0f;
		TimeScale = 0.8f;
		WaveAmplitude = 0.35f;
		WindDirection = FVector2D(0.8f, 0.6f);
		WindSpeed = 600.0f;
		WindDependency = 0.07f;
		ChoppyScale = 1.3f;
	}
};

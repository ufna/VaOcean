// Copyright 2014 Vladimir Alyamkin. All Rights Reserved.

#pragma once

#include "VaOceanStateActorSimple.generated.h"

/**
 * Calculates wave height based on SK_Ocean shader approach
 */
UCLASS(ClassGroup = VaOcean, Blueprintable, BlueprintType)
class AVaOceanStateActorSimple : public AVaOceanStateActor
{
	GENERATED_UCLASS_BODY()

	// Begin AVaOceanStateActor interface
	virtual float GetOceanLevelAtLocation(FVector& Location) const override;
	virtual FLinearColor GetOceanSurfaceNormal(FVector& Location) const override;
	virtual FVector GetOceanWaveVelocity(FVector& Location) const override;
	int32 GetOceanWavesNum() const override;
	// End AVaOceanStateActor interface

	//////////////////////////////////////////////////////////////////////////
	// Simple ocean model API

	/** SKOcean: WorldPositionDivider param */
	UFUNCTION(BlueprintCallable, Category = "World|VaOcean")
	float GetWorldPositionDivider() const;

	/** SKOcean: WaveUVDivider param */
	UFUNCTION(BlueprintCallable, Category = "World|VaOcean")
	float GetWaveUVDivider() const;

	/** SKOcean: WaveHeightPannerX param */
	UFUNCTION(BlueprintCallable, Category = "World|VaOcean")
	float GetWaveHeightPannerX() const;

	/** SKOcean: WaveHeightPannerY param */
	UFUNCTION(BlueprintCallable, Category = "World|VaOcean")
	float GetWaveHeightPannerY() const;

	/** SKOcean: WaveHeight param */
	UFUNCTION(BlueprintCallable, Category = "World|VaOcean")
	float GetWaveHeight() const;

	/** SKOcean: WaterHeight param */
	UFUNCTION(BlueprintCallable, Category = "World|VaOcean")
	float GetWaterHeight() const;

	/** Set time for waves movement calculation */
	UFUNCTION(BlueprintCallable, Category = "World|VaOcean")
	void SetWaveHeightPannerTime(float Time);

	/** Normalmap which will be used to determite the wave height */
	UPROPERTY(EditAnywhere, Category = OceanSetup)
	class UTexture2D* OceanHeightMap;

	/** How much waves are defined by normal map. Affects XY wave velocity. */
	UPROPERTY(EditAnywhere, Category = OceanSetup)
	int32 HeightMapWaves;

	// Begin AActor interface
	virtual void PostInitializeComponents() override;
	// End AActor interface

protected:

	//////////////////////////////////////////////////////////////////////////
	// Cached data to prevent large memory operations each frame

	/** Flat to keep "uncompression" state */
	bool bRawDataReady;

	/** Decompressed PNG image */
	TArray<uint8> HeightMapRawData;

	/** Return pixel color from loaded raw data */
	FColor GetHeighMapPixelColor(float U, float V) const;


	//////////////////////////////////////////////////////////////////////////
	// Wave height calculation parameters

	UPROPERTY(EditAnywhere, Category = OceanSetup)
	float WorldPositionDivider;

	UPROPERTY(EditAnywhere, Category = OceanSetup)
	float WaveUVDivider;

	UPROPERTY(EditAnywhere, Category = OceanSetup)
	float WaveHeightPannerX;

	UPROPERTY(EditAnywhere, Category = OceanSetup)
	float WaveHeightPannerY;

	UPROPERTY(EditAnywhere, Category = OceanSetup)
	float WaveHeightPannerTime;

	UPROPERTY(EditAnywhere, Category = OceanSetup)
	float WaveHeight;

	UPROPERTY(EditAnywhere, Category = OceanSetup)
	float WaterHeight;

};

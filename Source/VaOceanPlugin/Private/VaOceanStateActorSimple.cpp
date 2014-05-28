// Copyright 2014 Vladimir Alyamkin. All Rights Reserved.

#include "VaOceanPluginPrivatePCH.h"

AVaOceanStateActorSimple::AVaOceanStateActorSimple(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	HeightMapWaves = 1;

	WorldPositionDivider = 0.5f;
	WaveUVDivider = 11000.0f;

	WaveHeightPannerX = 0.015f;
	WaveHeightPannerY = 0.01f;

	WaveHeight = 100.0f;
	WaterHeight = 100.0f;

	WaveHeightPannerTime = 0.0f;
}

void AVaOceanStateActorSimple::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	// Load raw data with first function call
	if (OceanHeightMap)
	{
#if WITH_EDITORONLY_DATA
		bRawDataReady = OceanHeightMap->Source.GetMipData(HeightMapRawData, 0);
#else
		bRawDataReady = false;
#endif
		UE_LOG(LogVaOcean, Log, TEXT("Ocean heighmap load status: %d"), (int)bRawDataReady);
	}
	else
	{
		UE_LOG(LogVaOcean, Warning, TEXT("Heightmap is not set for ocean state!"));
		bRawDataReady = false;
	}
}


//////////////////////////////////////////////////////////////////////////
// Ocean state API

float AVaOceanStateActorSimple::GetOceanLevelAtLocation(FVector& Location) const
{
	// Check that we've set a texture
	if (!OceanHeightMap)
	{
		return GetGlobalOceanLevel() + WaterHeight;
	}

#if WITH_EDITORONLY_DATA
	check(OceanHeightMap->Source.IsValid());

	// Check we have a raw data loaded
	if (!bRawDataReady)
	{
		// UE_LOG(LogVaOcean, Warning, TEXT("Can't load raw data of ocean heightmap!"));
		return 0.0f;
	}

	//
	// SimpleOcean shader algorythm
	//

	// World UV location
	float WorldUVx = Location.X / WorldPositionDivider / WaveUVDivider;
	float WorldUVy = Location.Y / WorldPositionDivider / WaveUVDivider;

	// Apply panner
	WorldUVx += WaveHeightPannerX * WaveHeightPannerTime;
	WorldUVy += WaveHeightPannerY * WaveHeightPannerTime;

	// Get heightmap color
	const FLinearColor PixelColor = FLinearColor(GetHeighMapPixelColor(WorldUVx, WorldUVy));

	// Calculate wave height
	float OceanLevel = PixelColor.A * WaveHeight - WaterHeight;

	return OceanLevel + GlobalOceanLevel;
#else
	return GetGlobalOceanLevel() + WaterHeight;
#endif
}

FLinearColor AVaOceanStateActorSimple::GetOceanSurfaceNormal(FVector& Location) const
{
	// Check that we've set a texture
	if (!OceanHeightMap)
	{
		return FLinearColor::Blue;
	}

#if WITH_EDITORONLY_DATA
	check(OceanHeightMap->Source.IsValid());

	// Check we have a raw data loaded
	if (!bRawDataReady)
	{
		// UE_LOG(LogVaOcean, Warning, TEXT("Can't load raw data of ocean heightmap!"));
		return FLinearColor::Black;
	}

	//
	// SKOcean shader algorythm
	//

	// World UV location
	float WorldUVx = Location.X / WorldPositionDivider / WaveUVDivider;
	float WorldUVy = Location.Y / WorldPositionDivider / WaveUVDivider;

	// Apply panner
	WorldUVx += WaveHeightPannerX * WaveHeightPannerTime;
	WorldUVy += WaveHeightPannerY * WaveHeightPannerTime;

	// Get heightmap color
	return FLinearColor(GetHeighMapPixelColor(WorldUVx, WorldUVy));
#else
	return FLinearColor::Blue;
#endif
}

FVector AVaOceanStateActorSimple::GetOceanWaveVelocity(FVector& Location) const
{
	FVector WaveVelocity = FVector(WaveHeightPannerX, WaveHeightPannerY, 0.0f);

	// Scale to the world size (in m/sec!)
	WaveVelocity *= WorldPositionDivider * WaveUVDivider / 100.0f;

	return WaveVelocity;
}

int32 AVaOceanStateActorSimple::GetOceanWavesNum() const
{
	return HeightMapWaves;
}

FColor AVaOceanStateActorSimple::GetHeighMapPixelColor(float U, float V) const
{
	// Check we have a raw data loaded
	if (!bRawDataReady)
	{
		//UE_LOG(LogVaOcean, Warning, TEXT("Ocean heightmap raw data is not loaded! Pixel is empty."));
		return FColor::Black;
	}

#if WITH_EDITORONLY_DATA
	// We are using the source art so grab the original width/height
	const int32 Width = OceanHeightMap->Source.GetSizeX();
	const int32 Height = OceanHeightMap->Source.GetSizeY();
	const bool bUseSRGB = OceanHeightMap->SRGB;

	check(Width > 0 && Height > 0 && HeightMapRawData.Num() > 0);

	// Normalize UV first
	const float NormalizedU = U > 0 ? FMath::Fractional(U) : 1.0 + FMath::Fractional(U);
	const float NormalizedV = V > 0 ? FMath::Fractional(V) : 1.0 + FMath::Fractional(V);

	const int PixelX = NormalizedU * (Width-1) + 1;
	const int PixelY = NormalizedV * (Height-1) + 1;

	// Get color from
	const FColor* SrcPtr = &((FColor*)(HeightMapRawData.GetTypedData()))[(PixelY - 1) * Width + PixelX - 1];

	return *SrcPtr;
#else
	return FColor::Black;
#endif
}

//////////////////////////////////////////////////////////////////////////
// Parameters access (get/set)

float AVaOceanStateActorSimple::GetWorldPositionDivider() const
{
	return WorldPositionDivider;
}

float AVaOceanStateActorSimple::GetWaveUVDivider() const
{
	return WaveUVDivider;
}

float AVaOceanStateActorSimple::GetWaveHeightPannerX() const
{
	return WaveHeightPannerX;
}

float AVaOceanStateActorSimple::GetWaveHeightPannerY() const
{
	return WaveHeightPannerY;
}

float AVaOceanStateActorSimple::GetWaveHeight() const
{
	return WaveHeight;
}

float AVaOceanStateActorSimple::GetWaterHeight() const
{
	return WaterHeight;
}

void AVaOceanStateActorSimple::SetWaveHeightPannerTime(float Time)
{
	WaveHeightPannerTime = Time;
}

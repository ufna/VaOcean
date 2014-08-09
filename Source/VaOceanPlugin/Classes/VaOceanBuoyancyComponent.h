// Copyright 2014 Vladimir Alyamkin. All Rights Reserved.

#pragma once

#include "GameFramework/PawnMovementComponent.h"
#include "VaOceanBuoyancyComponent.generated.h"

/**
 * Allows actor to swim in ocean
 */
UCLASS(ClassGroup = Environment, editinlinenew, meta = (BlueprintSpawnableComponent))
class UVaOceanBuoyancyComponent : public UMovementComponent
{
	GENERATED_UCLASS_BODY()

	/** Mass to set the vehicle chassis to. It's much easier to tweak vehicle settings when
	 * the mass doesn't change due to tweaks with the physics asset. [kg] */
	UPROPERTY(EditAnywhere, Category = VehicleSetup)
	float Mass;

	/** Default ocean level if we can't get it from map */
	UPROPERTY(EditAnywhere, Category = VehicleSetup)
	float OceanLevel;

	/** override center of mass offset, makes tweaking easier [uu] */
	UPROPERTY(EditAnywhere, Category = VehicleSetup, AdvancedDisplay)
	FVector COMOffset;
	
	//Begin UActorComponent Interface
	virtual void InitializeComponent() override;
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) override;
	//End UActorComponent Interface

protected:
	/** Wave reaction */
	virtual void PerformWaveReaction(float DeltaTime);

	/** Additional math */
	static void GetAxes(FRotator A, FVector& X, FVector& Y, FVector& Z);

	/** Ocean level at particular world position */
	float GetOceanLevel(FVector& WorldLocation) const;

	/** Ocean level difference (altitude) at desired position */
	float GetAltitude(FVector& WorldLocation) const;

	/** Ocean surface normal with altitude  */
	FLinearColor GetSurfaceNormal(FVector& WorldLocation) const;

	/** Horizontal wave velocity */
	FVector GetWaveVelocity(FVector& WorldLocation) const;

	/** How much to scale surface normal to get movement value */
	int32 GetSurfaceWavesNum() const;

protected:

	/** Get the local COM offset */
	virtual FVector GetCOMOffset();

protected:
	//
	// WAVE REACTION CONTROL
	//

	/** Position of ship's pompons */
	UPROPERTY(EditAnywhere, Category = WaveReaction)
	TArray<FVector> TensionDots;

	/** Factor applied to calculate depth-based point velocity */
	UPROPERTY(EditAnywhere, Category = WaveReaction)
	float TensionDepthFactor;

	/** Factor applied to calculate roll */
	UPROPERTY(EditAnywhere, Category = WaveReaction)
	float TensionTorqueFactor;

	/** Factor applied to calculate roll */
	UPROPERTY(EditAnywhere, Category = WaveReaction)
	float TensionTorqueRollFactor;

	/** Factor applied to calculate pitch */
	UPROPERTY(EditAnywhere, Category = WaveReaction)
	float TensionTorquePitchFactor;

	/** Makes tension dots visible */
	UPROPERTY(EditAnywhere, Category = WaveReaction)
	bool bDebugTensionDots;

	/** Attn.! Set only height. X and Y should be zero! */
	UPROPERTY(EditAnywhere, Category = WaveReaction)
	FVector LongitudinalMetacenter;

	/** Attn.! Set only height. X and Y should be zero! */
	UPROPERTY(EditAnywhere, Category = WaveReaction)
	FVector TransverseMetacenter;

	/** Use metacentric forces or not */
	UPROPERTY(EditAnywhere, Category = WaveReaction)
	bool bUseMetacentricForces;

	/** Factor applied to altitude to calc up force */
	UPROPERTY(EditAnywhere, Category = WaveReaction)
	float AltitudeFactor;

	/** Factor applied to velocity to calc up force fading */
	UPROPERTY(EditAnywhere, Category = WaveReaction)
	float VelocityFactor;

	/** Up force limit */
	UPROPERTY(EditAnywhere, Category = WaveReaction)
	float MaxAltitudeForce;

	/** Minimum altitude to react on the difference */
	UPROPERTY(EditAnywhere, Category = WaveReaction)
	float MinimumAltituteToReact;

private:

	/** Cached ocean state actor to avoid search each frame with ship */
	TWeakObjectPtr<AVaOceanStateActor> OceanStateActor;

};

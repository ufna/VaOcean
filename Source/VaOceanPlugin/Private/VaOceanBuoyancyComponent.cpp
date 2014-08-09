// Copyright 2014 Vladimir Alyamkin. All Rights Reserved.

#include "VaOceanPluginPrivatePCH.h"

UVaOceanBuoyancyComponent::UVaOceanBuoyancyComponent(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	bWantsInitializeComponent = true;

	Mass = 1500.0f;
	OceanLevel = 0.0f;
	// InertiaTensorScale = FVector(1.0f, 0.80f, 1.0f);

	TensionTorqueFactor = 0.01f;
	TensionTorqueRollFactor = 10000000.0f;
	TensionTorquePitchFactor = 30.0f;	// Pitch is mostly controlled by tesntion dots
	TensionDepthFactor = 100.0f;

	AltitudeFactor = 10.0f;
	VelocityFactor = 1.0f;

	MaxAltitudeForce = 600.0f;
	MinimumAltituteToReact = 30.0f;

	bDebugTensionDots = false;
	bUseMetacentricForces = false;

	LongitudinalMetacenter = FVector(0.0f, 0.0f, 150.0f);
	TransverseMetacenter = FVector(0.0, 0.0, 50.0);

	UpdatedComponent = NULL;
}

void UVaOceanBuoyancyComponent::InitializeComponent()
{
	Super::InitializeComponent();

	// Find updated component (mesh of owner)
	USkeletalMeshComponent* SkeletalMesh = GetOwner()->FindComponentByClass<USkeletalMeshComponent>();
	if (SkeletalMesh != NULL)
	{
		UpdatedComponent = SkeletalMesh;
	}
	else
	{
		// Check static mesh
		UStaticMeshComponent* StaticMesh = GetOwner()->FindComponentByClass<UStaticMeshComponent>();
		if (StaticMesh != NULL)
		{
			UpdatedComponent = StaticMesh;
		}
	}

	if (UpdatedComponent == NULL)
	{
		UE_LOG(LogVaOceanPhysics, Warning, TEXT("Can't find updated component for bouyancy actor!"));
	}

	// Find ocean state actor on scene
	TActorIterator< AVaOceanStateActor > ActorItr = TActorIterator< AVaOceanStateActor >(GetWorld());
	while (ActorItr)
	{
		OceanStateActor = *ActorItr;

		// Don't look for next actor, we need just one!
		break;
		//++ActorItr;
	}

	if (OceanStateActor.IsValid())
	{
		UE_LOG(LogVaOceanPhysics, Log, TEXT("Ocean state successfully found by GameState"));
	}
	else
	{
		UE_LOG(LogVaOceanPhysics, Warning, TEXT("Can't find ocean state actor! Default ocean level will be used."));
	}
}

void UVaOceanBuoyancyComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if(!UpdatedComponent)
	{
		return;
	}

	// React on world
	PerformWaveReaction(DeltaTime);
}

void UVaOceanBuoyancyComponent::PerformWaveReaction(float DeltaTime)
{
	AActor* MyOwner = GetOwner();

	if (!UpdatedComponent || MyOwner == NULL)
	{
		return;
	}

	const FVector OldLocation = MyOwner->GetActorLocation();
	const FRotator OldRotation = MyOwner->GetActorRotation();
	const FVector OldLinearVelocity = UpdatedComponent->GetPhysicsLinearVelocity();
	const FVector OldAngularVelocity = UpdatedComponent->GetPhysicsAngularVelocity();
	const FVector OldCenterOfMassWorld = OldLocation + OldRotation.RotateVector(COMOffset);
	const FVector OwnerScale = MyOwner->GetActorScale();

	// XYZ === Throttle, Steering, Rise == Forwards, Sidewards, Upwards
	FVector X, Y, Z;
	GetAxes(OldRotation, X, Y, Z);

	// Process tension dots and get torque from wind/waves
	for (FVector TensionDot : TensionDots)
	{
		// Translate point to world coordinates
		FVector TensionDotDisplaced = OldRotation.RotateVector(TensionDot + COMOffset);
		FVector TensionDotWorld = OldLocation + TensionDotDisplaced;

		// Get point depth
		float DotAltitude = GetAltitude(TensionDotWorld);

		// Don't process dots above water
		if (DotAltitude > 0)
		{
			continue;
		}
		
		// Surface normal (not modified!)
		FVector DotSurfaceNormal = GetSurfaceNormal(TensionDotWorld) * GetSurfaceWavesNum();
		// Modify normal with real Z value and normalize it
		DotSurfaceNormal.Z = GetOceanLevel(TensionDotWorld);
		DotSurfaceNormal.Normalize();

		// Point dynamic pressure [http://en.wikipedia.org/wiki/Dynamic_pressure]
		// rho = 1.03f for ocean water
		FVector WaveVelocity = GetWaveVelocity(TensionDotWorld);
		float DotQ = 0.515f * FMath::Square(WaveVelocity.Size());
		FVector WaveForce = FVector(0.0,0.0,1.0) * DotQ /* DotSurfaceNormal*/ * (-DotAltitude) * TensionDepthFactor;
		
		// We don't want Z to be affected by DotQ
		WaveForce.Z /= DotQ;

		// Scale to DeltaTime to break FPS addiction
		WaveForce *= DeltaTime;

		// Apply actor scale
		WaveForce *= OwnerScale.X;// *OwnerScale.Y * OwnerScale.Z;

		UpdatedComponent->AddForceAtLocation(WaveForce * Mass, TensionDotWorld);
	}

	// Static metacentric forces (can be useful on small waves)
	if (bUseMetacentricForces)
	{
		FVector TensionTorqueResult = FVector(0.0f, 0.0f, 0.0f);

		// Calc recovering torque (transverce)
		FRotator RollRot = FRotator(0.0f, 0.0f, 0.0f);
		RollRot.Roll = OldRotation.Roll;
		FVector MetacenterDisplaced = RollRot.RotateVector(TransverseMetacenter + COMOffset);
		TensionTorqueResult += X * FVector::DotProduct((TransverseMetacenter - MetacenterDisplaced), 
			FVector(0.0f, -1.0f, 0.0f)) * TensionTorqueRollFactor;

		// Calc recovering torque (longitude)
		FRotator PitchRot = FRotator(0.0f, 0.0f, 0.0f);
		PitchRot.Pitch = OldRotation.Pitch;
		MetacenterDisplaced = PitchRot.RotateVector(LongitudinalMetacenter + COMOffset);
		TensionTorqueResult += Y * FVector::DotProduct((LongitudinalMetacenter - MetacenterDisplaced), 
			FVector(1.0f, 0.0f, 0.0f)) * TensionTorquePitchFactor;

		// Apply torque
		TensionTorqueResult *= DeltaTime;
		TensionTorqueResult *= OwnerScale.X;// *OwnerScale.Y * OwnerScale.Z;
		UpdatedComponent->AddTorque(TensionTorqueResult);
	}
}

void UVaOceanBuoyancyComponent::GetAxes(FRotator A, FVector& X, FVector& Y, FVector& Z)
{
	FRotationMatrix R(A);
	R.GetScaledAxes(X, Y, Z);
}

FVector UVaOceanBuoyancyComponent::GetCOMOffset()
{
	return COMOffset;
}

float UVaOceanBuoyancyComponent::GetOceanLevel(FVector& WorldLocation) const
{
	if (OceanStateActor.IsValid())
	{
		return OceanStateActor->GetOceanLevelAtLocation(WorldLocation);
	}

	return OceanLevel;
}

float UVaOceanBuoyancyComponent::GetAltitude(FVector& WorldLocation) const
{
	return WorldLocation.Z - GetOceanLevel(WorldLocation) /*+ COMOffset.Z*/;
}

FLinearColor UVaOceanBuoyancyComponent::GetSurfaceNormal(FVector& WorldLocation) const
{
	if (OceanStateActor.IsValid())
	{
		return OceanStateActor->GetOceanSurfaceNormal(WorldLocation);
	}

	return FVector::UpVector;
}

int32 UVaOceanBuoyancyComponent::GetSurfaceWavesNum() const
{
	if (OceanStateActor.IsValid())
	{
		return OceanStateActor->GetOceanWavesNum();
	}

	return 1;
}

FVector UVaOceanBuoyancyComponent::GetWaveVelocity(FVector& WorldLocation) const
{
	if (OceanStateActor.IsValid())
	{
		return OceanStateActor->GetOceanWaveVelocity(WorldLocation);
	}

	return FVector::UpVector;
}

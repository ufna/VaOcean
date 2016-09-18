// Copyright 2014 Vladimir Alyamkin. All Rights Reserved.

#include "VaOceanPluginPrivatePCH.h"

AVaOceanStateActor::AVaOceanStateActor(const FObjectInitializer& PCIP)
	: Super(PCIP)
{
#if WITH_EDITORONLY_DATA
	// Structure to hold one-time initialization
	struct FConstructorStatics
	{
		// A helper class object we use to find target UTexture2D object in resource package
		ConstructorHelpers::FObjectFinderOptional<UTexture2D> NoteTextureObject;

		// Icon sprite category name
		FName ID_Notes;

		// Icon sprite display name
		FText NAME_Notes;

		FConstructorStatics()
			// Use helper class object to find the texture
			// "/Engine/EditorResources/S_Note" is resource path
			: NoteTextureObject(TEXT("/Engine/EditorResources/S_Note"))
			, ID_Notes(TEXT("Notes"))
			, NAME_Notes(NSLOCTEXT("SpriteCategory", "Notes", "Notes"))
		{
		}
	};
	static FConstructorStatics ConstructorStatics;
#endif // WITH_EDITORONLY_DATA

	// We need a scene component to attach Icon sprite
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("SceneComp"));
	RootComponent->Mobility = EComponentMobility::Static;

#if WITH_EDITORONLY_DATA
	SpriteComponent = CreateEditorOnlyDefaultSubobject<UBillboardComponent>(TEXT("Sprite"));
	if (SpriteComponent)
	{
		SpriteComponent->Sprite = ConstructorStatics.NoteTextureObject.Get();		// Get the sprite texture from helper class object
		SpriteComponent->SpriteInfo.Category = ConstructorStatics.ID_Notes;			// Assign sprite category name
		SpriteComponent->SpriteInfo.DisplayName = ConstructorStatics.NAME_Notes;	// Assign sprite display name
		SpriteComponent->SetupAttachment(RootComponent);							// Attach sprite to scene component						
		SpriteComponent->Mobility = EComponentMobility::Static;
	}
#endif // WITH_EDITORONLY_DATA

	OceanSimulator = NULL;

	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PrePhysics;
	SetRemoteRoleForBackwardsCompat(ROLE_SimulatedProxy);
	bReplicates = true;
}

void AVaOceanStateActor::PreInitializeComponents()
{
	OceanSimulator = FindComponentByClass<UVaOceanSimulatorComponent>();

	Super::PreInitializeComponents();
}

void AVaOceanStateActor::PostInitializeComponents()
{
	Super::PostInitializeComponents();
}


//////////////////////////////////////////////////////////////////////////
// Ocean state API

const FSpectrumData& AVaOceanStateActor::GetSpectrumConfig() const
{
	return SpectrumConfig;
}

float AVaOceanStateActor::GetOceanLevelAtLocation(FVector& Location) const
{
	// @TODO Get altitute from ocean simulation component

	return GetGlobalOceanLevel();
}

FLinearColor AVaOceanStateActor::GetOceanSurfaceNormal(FVector& Location) const
{
	// @TODO Get surface normals from ocean simulation component

	return FLinearColor::Blue;
}

FVector AVaOceanStateActor::GetOceanWaveVelocity(FVector& Location) const
{
	// @TODO Velocity should be calculated in ocean simulation component

	return FVector::UpVector;
}

int32 AVaOceanStateActor::GetOceanWavesNum() const
{
	return 1;
}

//////////////////////////////////////////////////////////////////////////
// Parameters access (get/set)

void AVaOceanStateActor::SetGlobalOceanLevel(float OceanLevel)
{
	GlobalOceanLevel = OceanLevel;
}

float AVaOceanStateActor::GetGlobalOceanLevel() const
{
	return GlobalOceanLevel;
}

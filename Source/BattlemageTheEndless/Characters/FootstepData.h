// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "FootstepData.generated.h"

USTRUCT(BlueprintType)
struct FFootstepMaterialProperties
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Footsteps, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UPhysicalMaterial> PhysicalMaterial;

	/** Key = Movement Speed (magnitude, can be negative), Value = A sound cue **/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Footsteps, meta = (AllowPrivateAccess = "true"))
	TMap<float, USoundCue*> FootstepSounds;
};

/**
 * This class is intended to hold data related to footstep effects, such as sounds and particle effects.
 * FoostepData is scoped to a single FootstepData (BP) ActorComponent so that different characters can have different footstep effects.
 */
UCLASS(BlueprintType, Blueprintable)
class BATTLEMAGETHEENDLESS_API UFootstepData: public UObject
{
	GENERATED_BODY()

public:
	UFootstepData();
	~UFootstepData();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Footsteps, meta = (AllowPrivateAccess = "true"))
	TArray<FFootstepMaterialProperties> FootstepMaterialProperties;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Footsteps, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USoundAttenuation> AttenuationSettings;

	UFUNCTION(BlueprintCallable, Category = Footsteps)
	USoundCue* GetFootstepSoundForMaterialAndSpeed(UPhysicalMaterial* PhysicalMaterial, float MovementSpeed);

private:
	bool _hasBeenSorted = false;
};

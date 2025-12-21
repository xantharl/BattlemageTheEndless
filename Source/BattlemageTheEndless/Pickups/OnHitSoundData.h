// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Abilities/GameplayAbility.h"
#include "UObject/Object.h"
#include "OnHitSoundData.generated.h"

/**
 * 
 */
USTRUCT(BlueprintType)
struct FHitSoundMaterialProperties
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = HitSounds, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UPhysicalMaterial> PhysicalMaterial;

	/** Tags that must be present on the ability to use these sounds, intended to differentiate heavy vs. light **/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = HitSounds, meta = (AllowPrivateAccess = "true"))
	FGameplayTagContainer RequiredAbilityTags;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = HitSounds, meta = (AllowPrivateAccess = "true"))
	USoundBase* HitSound;
};

UCLASS(BlueprintType, Blueprintable)
class BATTLEMAGETHEENDLESS_API UOnHitSoundData : public UObject
{
	GENERATED_BODY()
	
public:
	UOnHitSoundData();
	~UOnHitSoundData();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Footsteps, meta = (AllowPrivateAccess = "true"))
	TArray<FHitSoundMaterialProperties> HitSoundMaterialProperties;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Footsteps, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USoundAttenuation> AttenuationSettings;

	UFUNCTION(BlueprintCallable, Category = Footsteps)
	USoundBase* GetHitSoundForMaterialAndAbility(UPhysicalMaterial* PhysicalMaterial, UGameplayAbility* Ability);
};

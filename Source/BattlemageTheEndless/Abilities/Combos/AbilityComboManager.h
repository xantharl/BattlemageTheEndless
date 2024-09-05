// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include <BattlemageTheEndless/Pickups/PickupActor.h>
#include "AbilityCombo.h"
#include "AbilityComboManager.generated.h"

USTRUCT(BlueprintType)
struct FPickupCombos
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Equipment, meta = (AllowPrivateAccess = "true"))
	TArray<UAbilityCombo*> Combos = TArray<UAbilityCombo*>();

	UAbilityCombo* ActiveCombo;
};

/**
 * 
 */
UCLASS()
class BATTLEMAGETHEENDLESS_API UAbilityComboManager : public UObject
{
	GENERATED_BODY()
	
protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Equipment, meta = (AllowPrivateAccess = "true"))
	TMap<APickupActor*, FPickupCombos> Combos;

public:
	// Identifies any combos that the current pickup can perform
	void ParseCombos(APickupActor* PickupActor, UAbilitySystemComponent* abilitySystemComponent);
};

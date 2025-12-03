// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BattlemageTheEndless/Pickups/PickupActor.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "WidgetHelperFunctions.generated.h"

/**
 * 
 */
UCLASS()
class BATTLEMAGETHEENDLESS_API UWidgetHelperFunctions : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "WidgetHelperFunctions")
	static APickupActor* GetPickupBySpellClassName(FText SpellClassName, TArray<APickupActor*> AllPickups);
	
	UFUNCTION(BlueprintCallable, Category = "WidgetHelperFunctions")
	static FName GetAbilityFriendlyName(TSubclassOf<UGameplayAbility> AbilityClass);
};

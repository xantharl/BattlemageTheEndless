// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectExecutionCalculation.h"
#include <BattlemageTheEndless/BMageAbilitySystemComponent.h>
#include "Components/SphereComponent.h"
#include "ExecutionCalculation_Marked.generated.h"

/**
 * 
 */
UCLASS()
class BATTLEMAGETHEENDLESS_API UExecutionCalculation_Marked : public UGameplayEffectExecutionCalculation
{
	GENERATED_BODY()
	
	UExecutionCalculation_Marked();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Marked", meta = (AllowPrivateAccess = "true"))
	UMaterialInstance* MarkedMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Marked", meta = (AllowPrivateAccess = "true"))
	UMaterialInstance* MarkedMaterialOutOfRange;

	virtual void Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, OUT FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const override;
};

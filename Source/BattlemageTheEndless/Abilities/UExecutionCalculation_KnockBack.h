// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GameplayEffectExecutionCalculation.h"
#include "GameplayAbilities/Public/AbilitySystemComponent.h"
#include "UExecutionCalculation_KnockBack.generated.h"

/**
 * 
 */
UCLASS()
class BATTLEMAGETHEENDLESS_API UUExecutionCalculation_KnockBack : public UGameplayEffectExecutionCalculation
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "KnockBack", meta = (AllowPrivateAccess = "true"))
	FVector KnockBackVelocity = FVector(0.f, 0.f, 600.f);
	
	virtual void Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, OUT FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const override;
};

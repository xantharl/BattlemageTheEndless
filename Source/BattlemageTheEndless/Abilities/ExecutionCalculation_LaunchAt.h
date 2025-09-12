// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectExecutionCalculation.h"
#include "GameplayAbilities/Public/AbilitySystemComponent.h"
#include "Camera/CameraComponent.h"
#include "../Characters/BattlemageTheEndlessCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "ExecutionCalculation_LaunchAt.generated.h"

/**
 * 
 */
UCLASS()
class BATTLEMAGETHEENDLESS_API UExecutionCalculation_LaunchAt : public UGameplayEffectExecutionCalculation
{
	GENERATED_BODY()
	
	UExecutionCalculation_LaunchAt();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Launch", meta = (AllowPrivateAccess = "true"))
	float LaunchRange = 1000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Launch", meta = (AllowPrivateAccess = "true"))
	float LaunchSpeed = 1000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Targeting", meta = (AllowPrivateAccess = "true"))
	float TargetingToleranceDeg = 15.f;

	virtual void Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, OUT FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const override;
};

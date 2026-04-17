// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GameplayEffectExecutionCalculation.h"
#include "GameplayAbilities/Public/AbilitySystemComponent.h"
#include <BattlemageTheEndless/Characters/BMageCharacterMovementComponent.h>
#include <BattlemageTheEndless/Characters/BattlemageTheEndlessCharacter.h>
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

	/** If Disabled, knockback direction will only consider X and Y, then apply the Knock Back Velocity **/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "KnockBack", meta = (AllowPrivateAccess = "true"))
	bool ConsiderZAxis = false;

	/** If set, this calculation will trigger gravity scaling over time on the target, curve is in MS **/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "KnockBack", meta = (AllowPrivateAccess = "true"))
	UCurveFloat* GravityScaleOverTime = nullptr;
	
	/** If set, velocity will be zeroed after specified time, must be less than gravity curve duration **/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "KnockBack", meta = (AllowPrivateAccess = "true"))
	float CancelVelocityAfter = 0.f;
	
	virtual void Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, OUT FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const override;
	
	UFUNCTION()
	static FVector CancelVelocity(const ACharacter* Character);
private:

	mutable FTimerHandle CancelVelocityTimerHandle;
};

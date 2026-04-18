// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GameplayAbilities/Public/Abilities/GameplayAbilityTypes.h"
#include "GameFramework/Character.h"
#include <BattlemageTheEndless/Characters/BMageCharacterMovementComponent.h>
#include <BattlemageTheEndless/Characters/BattlemageTheEndlessCharacter.h>
#include "GA_Launched.generated.h"

/**
 * Applies a directional knockback launch to the owning character.
 * Intended to be triggered via a gameplay event; TriggerEventData.Instigator is the source actor.
 */
UCLASS()
class BATTLEMAGETHEENDLESS_API UGA_Launched : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_Launched();

	/** Base knockback velocity applied in the source-to-target direction */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "KnockBack")
	FVector KnockBackVelocity = FVector(0.f, 0.f, 600.f);

	/** If disabled, knockback direction only considers X and Y, then applies KnockBackVelocity */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "KnockBack")
	bool ConsiderZAxis = false;

	/** If set, triggers gravity scaling over time on the target; curve X-axis is seconds */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "KnockBack")
	UCurveFloat* GravityScaleOverTime = nullptr;

	/** If > 0, zeroes velocity after this many seconds (must be less than gravity curve duration) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "KnockBack")
	float CancelVelocityAfter = 0.f;


	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	UFUNCTION()
	void OnGravityCurveEnded();

private:
	mutable FTimerHandle CancelVelocityTimerHandle;
	float _savedMaxDepenetration = 0.f;

	static void CancelVelocity(ACharacter* Character);
};

// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GameplayEffect.h"
#include "../Pickups/BattlemageTheEndlessProjectile.h"
#include "Kismet/GameplayStatics.h"
#include "../Characters/BattlemageTheEndlessCharacter.h"
#include "GameFramework/Actor.h"
#include "Engine/EngineTypes.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEffectRemoved.h"
#include "AttackBaseGameplayAbility.generated.h"

class UNiagaraSystem;

/**
 * 
 */
UCLASS()
class BATTLEMAGETHEENDLESS_API UAttackBaseGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UAttackBaseGameplayAbility()
	{
		InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
		NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
	}

	/** Projectile class to spawn */
	UPROPERTY(EditDefaultsOnly, Category = Projectile)
	TSubclassOf<class ABattlemageTheEndlessProjectile> ProjectileClass;

	/** Sound to play each time we fire */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	USoundBase* FireSound;

	/** AnimMontage to play each time we fire */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	UAnimMontage* FireAnimation;

	/** Plays between combo stages if needed (next attack not requested yet) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	UAnimMontage* ComboPauseAnimation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Effects, meta = (AllowPrivateAccess = "true"))
	TArray<TSubclassOf<UGameplayEffect>> EffectsToApply;

	TObjectPtr<AActor> Owner;

	FGameplayTag GetAbilityName();
	bool HasComboTag();

	bool IsFirstInCombo();

	FGameplayTagContainer GetComboTags();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData);
	void SpawnProjectile(const FGameplayAbilityActorInfo* ActorInfo, ABattlemageTheEndlessCharacter* character, UWorld* const world);
	void UpdateComboState(ABattlemageTheEndlessCharacter* character);
	virtual void CancelAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateCancelAbility) override;
	void ResetTimerAndClearEffects(const FGameplayAbilityActorInfo* ActorInfo);
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	/// <summary>
	/// Play the animation used for a pause between combo stages if present
	/// </summary>
	/// <param name="ActorInfo"></param>
	void TryPlayComboPause(const FGameplayAbilityActorInfo* ActorInfo);

	UFUNCTION()
	void OnMontageCancelled();

	UFUNCTION()
	void OnEffectRemoved(const FGameplayEffectRemovalInfo& GameplayEffectRemovalInfo);

	UFUNCTION()
	void OnMontageCompleted();

protected:
	FTimerHandle EndTimerHandle;

	TArray<FActiveGameplayEffectHandle> ActiveEffectHandles;

};

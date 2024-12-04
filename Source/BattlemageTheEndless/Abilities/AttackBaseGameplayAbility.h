// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GameplayEffect.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Actor.h"
#include "Engine/EngineTypes.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEffectRemoved.h"
#include "GameFramework/Character.h"
#include "../Pickups/BattlemageTheEndlessProjectile.h"
#include "GameplayAbilities/Public/AbilitySystemComponent.h"
#include "HitScanChainEffect.h"
#include "InputTriggers.h"
#include <chrono>
#include "GameplayEffect.h"
#include "AttackBaseGameplayAbility.generated.h"

class UNiagaraSystem;
using namespace std::chrono;

/// <summary>
/// Enum of possible spell hit types
/// </summary>
UENUM(BlueprintType)
enum class HitType : uint8
{
	// For abilities that don't hit anything
	None UMETA(DisplayName = "None"),
	// Buffs or other effects that only affect the caster
	Self UMETA(DisplayName = "Self"),
	// Uses an AnimTrace notify to determine hit
	Melee UMETA(DisplayName = "Melee"),
	// Uses a line trace to determine hit
	HitScan UMETA(DisplayName = "HitScan"),
	// Uses one or more projectiles to determine hit
	Projectile UMETA(DisplayName = "Projectile")
};

/**
 * 
 */
UCLASS()
class BATTLEMAGETHEENDLESS_API UAttackBaseGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()

public:
	const float DEFAULT_MAX_DISTANCE = 10000.f;

	UAttackBaseGameplayAbility()
	{
		InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
		NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
	}
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = HitBehavior)
	HitType HitType = HitType::None;

	/** Projectile class to spawn */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Projectiles)
	FProjectileConfiguration ProjectileConfiguration;

	/** Required charge duration before ability can be fired **/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CastBehavior)
	float ChargeDuration = 0.f;

	/** Current charge duration **/
	milliseconds CurrentChargeDuration;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = CastBehavior)
	float ChargeSpellBaseDamage = 5.f;

	/** Currently Active Multiplier at full charge **/
	UPROPERTY(BlueprintReadOnly, Category = CastBehavior)
	float CurrentChargeDamageMultiplier = 0.f;

	/** Damage Multiplier at full charge **/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CastBehavior)
	float FullChargeDamageMultiplier = 3.f;

	milliseconds ActivationTime;

	/** Amount of times an ability can chain, 0 = no chaining **/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ChainBehavior)
	int NumberOfChains = 0;

	// Might make this decrease per chain in the future, for now keep it simple
	/** Amount of times an ability can chain, 0 = no chaining **/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ChainBehavior)
	float ChainDistance = 0;

	/** Time in seconds to wait before applying the next chain **/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ChainBehavior)
	float ChainDelay = 0;

	/** Time in seconds to wait before applying the next chain **/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ChainBehavior)
	TObjectPtr<UNiagaraSystem> ChainSystem;

	/** Actor(s) to spawn on hit (e.g. fire surface, explosion, ice wall, etc.) **/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = HitBehavior)
	TSubclassOf<AActor> HitEffectActors;

	/** Maximum distance for the ability, 0 = unlimited */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Projectile, meta = (AllowPrivateAccess = "true"))
	float MaxRange = DEFAULT_MAX_DISTANCE;

	/** AnimMontage to play during ability charge up */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Animations)
	UAnimMontage* ChargeAnimation;

	/** AnimMontage to play when we fire the ability */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Animations)
	UAnimMontage* FireAnimation;

	/** Plays between combo stages if needed (next attack not requested yet) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Animations)
	UAnimMontage* ComboPauseAnimation;

	/** GameplayEffects to apply, target depends on HitType **/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Effects, meta = (AllowPrivateAccess = "true"))
	TArray<TSubclassOf<UGameplayEffect>> EffectsToApply;

	FGameplayTag GetAbilityName();
	bool HasComboTag();

	bool IsFirstInCombo();

	FGameplayTagContainer GetComboTags();

	/// <summary>
	/// override of the ActivateAbility method to apply effects and spawn projectiles
	/// </summary>
	/// <param name="Handle"></param>
	/// <param name="ActorInfo"></param>
	/// <param name="ActivationInfo"></param>
	/// <param name="TriggerEventData"></param>
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData);

	void CreateAndDispatchMontageTask();

	/// <summary>
	/// Helper to apply any effects owned by this ability to the target, which can be the same as the character if applying to self
	/// </summary>
	/// <param name="instigator">Character causing the effect(s)</param>
	/// <param name="target">Target of the effect(s)</param>
	/// <param name="effectCauser">EffectCauser is the actor that is the physical source of the effect</param>
	void ApplyEffects(AActor* target, UAbilitySystemComponent* targetAsc, AActor* instigator = nullptr, AActor* effectCauser = nullptr);

	/// <summary>
	/// Override of the CancelAbility method to clear effects (even if they still have duration) and reset the Combo Continuation timer
	/// </summary>
	/// <param name="Handle"></param>
	/// <param name="ActorInfo"></param>
	/// <param name="ActivationInfo"></param>
	/// <param name="bReplicateCancelAbility"></param>
	virtual void CancelAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateCancelAbility) override;

	/// <summary>
	/// Clears all effects caused by this ability and resets the Combo Continuation timer
	/// </summary>
	/// <param name="ActorInfo"></param>
	/// <param name="wasCancelled"></param>
	void ResetTimerAndClearEffects(const FGameplayAbilityActorInfo* ActorInfo, bool wasCancelled = false);

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

	bool WillCancelAbility(FGameplayAbilitySpec* OtherAbility);

	UFUNCTION(BlueprintCallable, Category="ChainBehavior")
	TArray<FTimerHandle>& GetChainTimerHandles() 
	{
		if (chainTimerHandles.Num() == 0)
		{
			for (int i = 0; i < NumberOfChains; i++)
			{
				chainTimerHandles.Add(FTimerHandle());
			}
		}

		return chainTimerHandles; 
	}

	bool IsCharged();

	virtual void HandleChargeProgress();

protected:
	FTimerHandle EndTimerHandle;

	TArray<FActiveGameplayEffectHandle> ActiveEffectHandles;

private:
	/** Timer handles for chained abilities, do not reference directly without ensuring it is initialized
		See GetChainTimerHandles **/
	TArray<FTimerHandle> chainTimerHandles;

	bool _bIsCharged = false;
};

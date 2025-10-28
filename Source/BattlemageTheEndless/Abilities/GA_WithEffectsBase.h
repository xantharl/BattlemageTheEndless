// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GameplayAbilities/Public/AbilitySystemComponent.h"
#include "GA_WithEffectsBase.generated.h"

/**
 * 
 */
UCLASS()
class BATTLEMAGETHEENDLESS_API UGA_WithEffectsBase : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_WithEffectsBase()
	{
		InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
		NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
	}

	// TODO: Enable ending ability early if all effects are destroyed (e.g. fire shield)
	/** GameplayEffects to apply, target depends on HitType, for placed abilities put effects on the HitEffectActor instead **/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Effects, meta = (AllowPrivateAccess = "true", EditConditionHides))
	TArray<TSubclassOf<UGameplayEffect>> EffectsToApply;	

	FGameplayTag GetAbilityName();

	/// <summary>
	/// override of the ActivateAbility method to apply effects and spawn projectiles
	/// </summary>
	/// <param name="Handle"></param>
	/// <param name="ActorInfo"></param>
	/// <param name="ActivationInfo"></param>
	/// <param name="TriggerEventData"></param>
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData);

	/// <summary>
	/// Helper to apply any effects owned by this ability to the target, which can be the same as the character if applying to self
	/// </summary>
	/// <param name="instigator">Character causing the effect(s)</param>
	/// <param name="target">Target of the effect(s)</param>
	/// <param name="effectCauser">EffectCauser is the actor that is the physical source of the effect</param>
	void ApplyEffects(AActor* target, UAbilitySystemComponent* targetAsc, AActor* instigator = nullptr, AActor* effectCauser = nullptr);

	/// <summary>
	/// Override this method to set any SetByCaller values on the specHandle before applying it
	/// </summary>
	/// <param name="effect"></param>
	/// <param name="specHandle"></param>
	/// <param name="effectCauser"></param>
	virtual void HandleSetByCaller(TSubclassOf<UGameplayEffect> effect, FGameplayEffectSpecHandle specHandle, AActor* effectCauser);

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

	// Helper using current ability instance's data to end itself if it is active (to prevent attempted end of CDO)
	void EndSelf();

	void EndAbilityByTimeout(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled);

	bool WillCancelAbility(FGameplayAbilitySpec* OtherAbility);

	TArray<TObjectPtr<UGA_WithEffectsBase>> GetAbilityActiveInstances(FGameplayAbilitySpec* spec);

	bool HasComboTag();

	bool IsFirstInCombo();

	FGameplayTagContainer GetComboTags();

protected:
	FTimerHandle EndTimerHandle;

	TArray<FActiveGameplayEffectHandle> ActiveEffectHandles;

};

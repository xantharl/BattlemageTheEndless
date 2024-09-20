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

protected:
	FTimerHandle EndTimerHandle;
};

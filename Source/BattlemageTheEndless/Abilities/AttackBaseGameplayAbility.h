// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "../Pickups/BattlemageTheEndlessProjectile.h"
#include "Kismet/GameplayStatics.h"
#include "../Characters/BattlemageTheEndlessCharacter.h"
#include "GameFramework/Actor.h"
#include "Engine/EngineTypes.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "AttackBaseGameplayAbility.generated.h"

class UNiagaraSystem;

USTRUCT(BlueprintType)
struct FAttackEffectData
{
	GENERATED_BODY()

	/// <summary>
	/// Effect to use for the attack
	/// </summary>
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Effects, meta = (AllowPrivateAccess = "true"))
	TSubclassOf<class UNiagaraSystem> NiagaraSystemClass;

	UNiagaraComponent* NiagaraComponentInstance;

	/// <summary>
	/// Offset for spawn of the effect, calculated from the camera's position
	/// </summary>
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Effects, meta = (AllowPrivateAccess = "true"))
	FVector SpawnOffset = FVector(500.f, 0.f, 0.f);

	/// <summary>
	/// Determines whether the effect should spawn at the offset location or on the ground below the offset location
	/// </summary>
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Effects, meta = (AllowPrivateAccess = "true"))
	bool bSnapToGround = false;

	/// <summary>
	/// Optional attachment socket if the effect should be attached to the character
	/// </summary>
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Effects, meta = (AllowPrivateAccess = "true"))
	FName AttachSocket;

	/// <summary>
	/// Determines whether the effect should kill the previous effect
	///		This is handled in UAbilityComboManager::ActivateAbilityAndResetTimer
	/// </summary>
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Effects, meta = (AllowPrivateAccess = "true"))
	bool bShouldKillPreviousEffect = false;
};

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Effects, meta = (AllowPrivateAccess = "true"))
	FAttackEffectData AttackEffect;

	/** Sound to play each time we fire */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	USoundBase* FireSound;

	/** AnimMontage to play each time we fire */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	UAnimMontage* FireAnimation;	

	TObjectPtr<AActor> Owner;

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData);
	void SpawnAttackEffect(const FGameplayAbilityActorInfo* ActorInfo, ABattlemageTheEndlessCharacter* character, UWorld* const world);
	void SpawnProjectile(const FGameplayAbilityActorInfo* ActorInfo, ABattlemageTheEndlessCharacter* character, UWorld* const world);
	void UpdateComboState(ABattlemageTheEndlessCharacter* character);
};

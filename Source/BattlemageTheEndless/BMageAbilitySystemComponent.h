// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "Abilities/BaseAttributeSet.h"
#include <Components/SphereComponent.h>
#include "Pickups/PickupActor.h"
#include "Abilities/Combos/AbilityComboManager.h"
#include "Abilities/AttackBaseGameplayAbility.h"
#include "Abilities/ProjectileManager.h"
#include <chrono>
#include "BMageAbilitySystemComponent.generated.h"

/**
 * 
 */
UCLASS()
class BATTLEMAGETHEENDLESS_API UBMageAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()

public:
	UBMageAbilitySystemComponent();

	void MarkOwner(AActor* instigator, float duration, float range, UMaterialInstance* markedMaterial, UMaterialInstance* markedMaterialOutOfRange);

	/** Handles attack input for pickups using GAS abilities **/
	UFUNCTION(BlueprintCallable, Category = "Combo Handler Passthru")
	void ProcessInputAndBindAbilityCancelled(APickupActor* PickupActor, EAttackType AttackType, ETriggerEvent triggerEvent);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UAbilityComboManager* ComboManager;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UProjectileManager* ProjectileManager;

private:
	void UnmarkOwner();

	void EnsureInitSubObjects();
	FTimerHandle _unmarkTimerHandle = FTimerHandle();

	USphereComponent* _markSphere;

	AActor* _instigator;
	UMaterialInstance* _markedMaterial;
	UMaterialInstance* _markedMaterialOutOfRange;

	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable, Category = "Hit Effect")
	virtual void OnMarkSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION(BlueprintCallable, Category = "Hit Effect")
	virtual void OnMarkSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	virtual void OnRemoveGameplayEffectCallback(const FActiveGameplayEffect& EffectRemoved);

	// return the first ability found with the specified owned tag
	TObjectPtr<UGameplayAbility> GetActivatableAbilityByOwnedTag(FName abilityTag);
	
	void PostAbilityActivation(UAttackBaseGameplayAbility* ability);

	void HandleProjectileSpawn(UAttackBaseGameplayAbility* ability);
	void HandleHitScan(UAttackBaseGameplayAbility* ability);
	TArray<ABattlemageTheEndlessCharacter*> GetChainTargets(int NumberOfChains, float ChainDistance, ABattlemageTheEndlessCharacter* HitActor);
	ABattlemageTheEndlessCharacter* GetNextChainTarget(float ChainDistance, AActor* ChainActor, TArray<AActor*> Candidates);
	void OnAbilityCancelled(const FAbilityEndedData& endData);
};

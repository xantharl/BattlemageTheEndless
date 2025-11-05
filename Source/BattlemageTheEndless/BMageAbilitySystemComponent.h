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
#include "Abilities/GE_RegenHealth.h"
#include "Abilities/GE_SuspendRegenHealth.h"
#include <chrono>
#include "BMageAbilitySystemComponent.generated.h"

UENUM(BlueprintType)
enum class EGASAbilityInputId : uint8
{
	None UMETA(DisplayName = "None"),
	Confirm UMETA(DisplayName = "Confirm"),
	Cancel UMETA(DisplayName = "Cancel")
};

/**
 * 
 */
UCLASS()
class BATTLEMAGETHEENDLESS_API UBMageAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()

public:
	UBMageAbilitySystemComponent();

	void DeactivatePickup(APickupActor* pickup);

	void ActivatePickup(APickupActor* activePickup);
	virtual bool GetShouldTick() const override;
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	void MarkOwner(AActor* instigator, float duration, float range, UMaterialInstance* markedMaterial, UMaterialInstance* markedMaterialOutOfRange);

	/** Handles attack input for pickups using GAS abilities **/
	UFUNCTION(BlueprintCallable, Category = "Combo Handler Passthru")
	void ProcessInputAndBindAbilityCancelled(APickupActor* PickupActor, EAttackType AttackType);

	UFUNCTION(BlueprintCallable, Category = "Abilities")
	bool CancelAbilityByOwnedTag(FGameplayTag abilityTag);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Abilities, meta = (AllowPrivateAccess = "true"), Instanced)
	UAbilityComboManager* ComboManager;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Abilities, meta = (AllowPrivateAccess = "true"))
	UProjectileManager* ProjectileManager;

	// Owned by character by character and set in ASC on init for convenience
	bool IsLeftHanded;

	/** Duration (s) to suspend health regeneration on hit, a value of 0 means do not suspend **/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Abilities, meta = (AllowPrivateAccess = "true"))
	float SuspendRegenOnHitDuration = 2.0f;

	TArray<APickupActor*> ActivePickups;
	UFUNCTION(BlueprintCallable, Category = "Pickup")
	APickupActor* GetActivePickup(EquipSlot Slot);

	void PostAbilityActivation(UAttackBaseGameplayAbility* ability);
	void HandleProjectileSpawn(UAttackBaseGameplayAbility* ability);
	void HandleHitScan(UAttackBaseGameplayAbility* ability);
	TArray<ACharacter*> GetChainTargets(int NumberOfChains, float ChainDistance, ACharacter* HitActor);
	ACharacter* GetNextChainTarget(float ChainDistance, AActor* ChainActor, TArray<AActor*> Candidates);
	void OnAbilityCancelled(const FAbilityEndedData& endData);

	UFUNCTION(BlueprintCallable, Category = "Abilities")
	void SuspendHealthRegen(float SuspendDurationOverride = 0.f);

	UFUNCTION(BlueprintCallable, Category = "Abilities")
	UAttackBaseGameplayAbility* BeginChargeAbility(TSubclassOf<UGameplayAbility> InAbilityClass);

	UFUNCTION(BlueprintCallable, Category = "Abilities")
	void CompleteChargeAbility();

	void ChargeSpell();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Abilities, meta = (AllowPrivateAccess = "true"))
	int ChargeTickRate = 60;

private:
	void UnmarkOwner();

	void EnsureInitSubObjects();
	FTimerHandle _unmarkTimerHandle = FTimerHandle();
	FTimerHandle _regenSuspendedTimerHandle = FTimerHandle();
	FTimerHandle _chargeSpellTimerHandle = FTimerHandle();

	UAttackBaseGameplayAbility* _chargingAbility = nullptr;

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
};

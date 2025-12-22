// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include <format>
#include "UObject/NoExportTypes.h"
#include "../GA_WithEffectsBase.h"
#include <BattlemageTheEndless/Pickups/PickupActor.h>
#include "AbilityCombo.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "BattlemageTheEndless/Characters/ComboManagerComponent.h"
#include "AbilityComboManager.generated.h"


/**
 * 
 */
UCLASS()
class BATTLEMAGETHEENDLESS_API UAbilityComboManager : public UObject
{
	GENERATED_BODY()
public:
	FGameplayAbilityActorInfo GetOwnerActorInfo();
	
protected:
	FTimerHandle ComboTimerHandle;

	int CurrentComboNumber = 0;

	UFUNCTION()
	void ActivateAbilityAndResetTimer(FGameplayAbilitySpec abilitySpec);

	void EndComboHandler();

	FGameplayAbilitySpecHandle* SwitchAndAdvanceCombo(APickupActor* PickupActor, UAbilityCombo* Combo);

	bool CheckCooldownAndTryActivate(FGameplayAbilitySpec abilitySpec);

	/// <summary>
	/// The combo manager allows for an action queue of 1, this is used in the event that input
	///  is received for an ability while one is ongoing
	/// </summary>
	FGameplayAbilitySpecHandle* NextAbilityHandle;

	FTimerHandle QueuedAbilityTimer;

public:
	FGameplayAbilitySpecHandle LastActivatedAbilityHandle;

	TObjectPtr<UNiagaraComponent> LastAbilityNiagaraInstance;

	void OnAbilityFailed(const UGameplayAbility* ability, const FGameplayTagContainer& reason);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Equipment, meta = (AllowPrivateAccess = "true"))
	TMap<APickupActor*, FPickupCombos> Combos;

	// Adds a single ability to a combo, or creates a new combo if the ability is the first in a combo
	void AddAbilityToCombo(APickupActor* PickupActor, UGA_WithEffectsBase* Ability, FGameplayAbilitySpecHandle Handle);

	/** Processes input from the player, can activate immediately or queue an action. Returns an invalid handle if no ability was activated. **/
	FGameplayAbilitySpecHandle ProcessInput(APickupActor* PickupActor, EAttackType AttackType);

	FGameplayAbilitySpecHandle DelegateToWeapon(APickupActor* PickupActor, EAttackType AttackType);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Combo, meta = (AllowPrivateAccess = "true"))
	// one second in std::chrono::milliseconds
	double ComboExpiryTime = 1.0;

	UAbilitySystemComponent* AbilitySystemComponent;

	UAbilityCombo* FindComboByTag(APickupActor* PickupActor, const FGameplayTag& ComboTag);

private:
	FGameplayAbilityActorInfo _ownerActorInfo = FGameplayAbilityActorInfo();
};

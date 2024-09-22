// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include <BattlemageTheEndless/Pickups/PickupActor.h>
#include "AbilityCombo.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "AbilityComboManager.generated.h"

USTRUCT(BlueprintType)
struct FPickupCombos
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Equipment, meta = (AllowPrivateAccess = "true"))
	TArray<UAbilityCombo*> Combos = TArray<UAbilityCombo*>();

	UAbilityCombo* ActiveCombo;
};

/**
 * 
 */
UCLASS()
class BATTLEMAGETHEENDLESS_API UAbilityComboManager : public UObject
{
	GENERATED_BODY()
	
protected:
	FTimerHandle ComboTimerHandle;

	int CurrentComboNumber = 0;

	UFUNCTION()
	void ActivateAbilityAndResetTimer(struct FGameplayAbilitySpecHandle& Ability);

	void EndComboHandler();

	FGameplayAbilitySpecHandle* SwitchAndAdvanceCombo(APickupActor* PickupActor, UAbilityCombo* Combo);

	FGameplayAbilitySpecHandle LastActivatedAbilityHandle;
	TObjectPtr<UNiagaraComponent> LastAbilityNiagaraInstance;

	/// <summary>
	/// The combo manager allows for an action queue of 1, this is used in the event that input
	///  is received for an ability while one is ongoing
	/// </summary>
	FGameplayAbilitySpecHandle* NextAbilityHandle;

	FTimerHandle QueuedAbilityTimer;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Equipment, meta = (AllowPrivateAccess = "true"))
	TMap<APickupActor*, FPickupCombos> Combos;

	// Adds a single ability to a combo, or creates a new combo if the ability is the first in a combo
	void AddAbilityToCombo(APickupActor* PickupActor, UAttackBaseGameplayAbility* Ability, FGameplayAbilitySpecHandle Handle);

	void ProcessInput(APickupActor* PickupActor, EAttackType AttackType);

	void DelegateToWeapon(APickupActor* PickupActor, EAttackType AttackType);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Combo, meta = (AllowPrivateAccess = "true"))
	// one second in std::chrono::milliseconds
	double ComboExpiryTime = 1.0;

	UAbilitySystemComponent* AbilitySystemComponent;
};

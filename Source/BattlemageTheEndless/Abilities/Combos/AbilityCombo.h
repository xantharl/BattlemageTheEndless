// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include <AbilitySystemComponent.h>
#include "AbilityCombo.generated.h"

/**
 * Class representing an ability combo which can be used by a character
 */
UCLASS()
class BATTLEMAGETHEENDLESS_API UAbilityCombo : public UObject
{
	GENERATED_BODY()

public:
	// returns the first ability spec in the combo after setting the last attack to 0
	FGameplayAbilitySpecHandle* StartCombo(int startAtAttackNumber = 0);

	// returns the next ability spec and increments the combo state counter
	FGameplayAbilitySpecHandle* AdvanceCombo();

	// This is the combo's base identifier e.g. Weapon.Light or Spells.Ice.Errupt
	// It is expected that ability intended for combos will also have a State.Combo.X tag
	FGameplayTag BaseComboIdentifier;

	// Sorts the AbilityHandles array by the ability's tag suffix
	// I think this will likely not do much if granted abilities are already in order but it's a safety measure
	void SortCombo();

	// resets the combo state
	void EndCombo();

	TArray<FGameplayAbilitySpecHandle> AbilityHandles;

	FORCEINLINE bool operator==(UAbilityCombo const& Other) const
	{
		return BaseComboIdentifier == Other.BaseComboIdentifier;
	}

	int LastComboAttackNumber;
};

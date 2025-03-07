// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilityCombo.h"

FGameplayAbilitySpecHandle* UAbilityCombo::StartCombo(int startAtAttackNumber)
{
    if (startAtAttackNumber + 1 > AbilityHandles.Num())
	{
		UE_LOG(LogTemp, Error, TEXT("Attempted to start combo at attack %d but combo only has %d attacks."), startAtAttackNumber, AbilityHandles.Num());
	}

	LastComboAttackNumber = startAtAttackNumber;
	return &AbilityHandles[startAtAttackNumber];
}

FGameplayAbilitySpecHandle* UAbilityCombo::AdvanceCombo()
{
	LastComboAttackNumber++;
	// We can't advance the combo if we're at the end
	if (LastComboAttackNumber >= AbilityHandles.Num())
	{
		EndCombo();
		return nullptr;
	}

	return &AbilityHandles[LastComboAttackNumber];
}

void UAbilityCombo::SortCombo()
{
	// TODO: Figure out if we need this
}

void UAbilityCombo::EndCombo()
{
	LastComboAttackNumber = 0;
}

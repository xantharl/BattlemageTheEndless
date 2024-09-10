// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilityComboManager.h"
#include "Abilities/GameplayAbility.h"
#include <BattlemageTheEndless/Abilities/AttackBaseGameplayAbility.h>

void UAbilityComboManager::AddAbilityToCombo(APickupActor* PickupActor, UGameplayAbility* Ability, FGameplayAbilitySpecHandle Handle)
{
	// We build combos for each pickup the first time it is equipped and assume they will not change during gameplay
	// TODO: If we want to allow for dynamic combos, we will need an event this can subscribe to for a rebuild
	if (!Combos.Contains(PickupActor))
	{
		Combos.Add(PickupActor, FPickupCombos());
	}

	// populate tags to look for
	FGameplayTagContainer comboStateTags = FGameplayTagContainer(FGameplayTag::RequestGameplayTag(FName("State.Combo")));
	FGameplayTagContainer baseComboIdentifierTags = FGameplayTagContainer(FGameplayTag::RequestGameplayTag(FName("Weapons")));
	baseComboIdentifierTags.AddTag(FGameplayTag::RequestGameplayTag(FName("Spells")));

	// create or add to a combo as needed
	auto ownedComboStateTags = Ability->AbilityTags.Filter(comboStateTags);
		
	// nothing to do if this ability isn't part of a combo
	if (ownedComboStateTags.Num() == 0)
		return;
	else if (ownedComboStateTags.Num() > 1)
	{
		UE_LOG(LogTemp, Warning, TEXT("Ability %s has more than one State.Combo tag. This is not supported."), *Ability->GetName());
		return;
	}

	// otherwise identify this ability's BaseComboIdentifier
	auto ownedBaseComboTags = Ability->AbilityTags.Filter(baseComboIdentifierTags);

	// it is expected that the resulting tag will be something like Weapons.Sword.LightAttack.1
	// meaning we want the direct parent to be the BaseComboIdentifier
	FGameplayTag parentTag = ownedBaseComboTags.First().GetGameplayTagParents().GetByIndex(1);
	UAbilityCombo* combo = nullptr;
	if (Combos[PickupActor].Combos.Num() > 0)
	{
		UAbilityCombo** searchResult = Combos[PickupActor].Combos.FindByPredicate([parentTag](const UAbilityCombo* combo) {return combo->BaseComboIdentifier == parentTag; });
		if (searchResult)
		{
			combo = *searchResult;
		}
	}

	if (combo)
	{			
		combo->AbilityHandles.Add(Handle);
	}
	else
	{
		UAbilityCombo* newCombo = NewObject<UAbilityCombo>();
		newCombo->BaseComboIdentifier = parentTag;
		newCombo->AbilityHandles.Add(Handle);
		Combos[PickupActor].Combos.Add(newCombo);
	}
}

void UAbilityComboManager::ProcessInput(APickupActor* PickupActor, EAttackType AttackType)
{
	// If we aren't aware of any combos for this pickup, delegate to the weapon's input handler
	if (!Combos.Contains(PickupActor))
	{
		DelegateToWeapon(PickupActor, AttackType);
		return;
	}

	FPickupCombos& pickupCombos = Combos[PickupActor];
	UAbilityCombo* combo;

	// TODO: This is kind of a hack to get spells working (since they don't have heavy/light combos)
	if(pickupCombos.Combos.Num() == 1)
	{
		combo = pickupCombos.Combos[0];
	}
	else
	{
		FString attackTypeName = *UEnum::GetDisplayValueAsText(AttackType).ToString();
		// otherwise we need to find the most applicable Combo based on the AttackType and then start or advance the combo
		TArray<UAbilityCombo*> matches = pickupCombos.Combos.FilterByPredicate(
			[attackTypeName](const UAbilityCombo* combo) {
				return combo->BaseComboIdentifier.GetTagName().ToString().Contains(attackTypeName); });

		// If the current attack isn't part of any combo, delegate to the weapon's input handler
		if (matches.Num() == 0)
		{
			DelegateToWeapon(PickupActor, AttackType);
			return;
		}
		else if (matches.Num() > 1)
		{
			UE_LOG(LogTemp, Warning, TEXT("More than one combo found for attack type %s. This is likely invalid configuration."), *attackTypeName);
		}
		combo = matches[0];
	}

	// if we're switching combos (e.g. heavy to light) we need to replace the active combo and advance the new one
	bool isComboSwap = pickupCombos.ActiveCombo && pickupCombos.ActiveCombo != combo;
	FGameplayAbilitySpecHandle* toActivate;
	if (isComboSwap)
	{
		toActivate = SwitchAndAdvanceCombo(PickupActor, combo);
	}
	else
	{
		toActivate = pickupCombos.ActiveCombo ? combo->AdvanceCombo() : combo->StartCombo();
	}

	pickupCombos.ActiveCombo = combo;
	// if we found a next ability, activate it
	if (toActivate)
		ActivateAbilityAndResetTimer(pickupCombos, toActivate);
	// if we didn't but we have a combo, we're at the end of the combo, restart it
	else if (combo)
	{
		ActivateAbilityAndResetTimer(pickupCombos, combo->StartCombo());
	}
}

void UAbilityComboManager::DelegateToWeapon(APickupActor* PickupActor, EAttackType AttackType)
{
	auto abilityClass = PickupActor->Weapon->GetAbilityByAttackType(AttackType);
	if (!abilityClass)
		return;
	AbilitySystemComponent->TryActivateAbility(
		AbilitySystemComponent->FindAbilitySpecFromClass(abilityClass)->Handle, true);
}

void UAbilityComboManager::ActivateAbilityAndResetTimer(FPickupCombos ComboData, FGameplayAbilitySpecHandle* Ability)
{
	// handle request to kill previous effect
	auto abilitySpec = Cast<UAttackBaseGameplayAbility>(AbilitySystemComponent->FindAbilitySpecFromHandle(*Ability)->Ability);
	if (abilitySpec && abilitySpec->AttackEffect.bShouldKillPreviousEffect)
	{
		auto lastAbilitySpec = Cast<UAttackBaseGameplayAbility>(AbilitySystemComponent->FindAbilitySpecFromHandle(*LastActivatedAbilityHandle)->Ability);
		if (lastAbilitySpec->AttackEffect.NiagaraComponentInstance)
			lastAbilitySpec->AttackEffect.NiagaraComponentInstance->DeactivateImmediate();
	}

	LastActivatedAbilityHandle = Ability;

	if (GEngine)
	{
		FString abilityName = AbilitySystemComponent->FindAbilitySpecFromHandle(*Ability)->Ability->GetName();
		GEngine->AddOnScreenDebugMessage(-1, 1.50f, FColor::Yellow, FString::Printf(TEXT("Activating ability %s"), *abilityName));
	}

	AbilitySystemComponent->TryActivateAbility(*Ability, true);
	// intentionally overwrite the timer handle each time we advance the combo
	GetWorld()->GetTimerManager().SetTimer(ComboTimerHandle, [&, ComboData] {EndComboHandler(); }, ComboExpiryTime, false);
}

void UAbilityComboManager::EndComboHandler() 
{
	// taking the lazy approach and ending any and all combos, this may change later
	if (Combos.Num() == 0)
		return;
	for (auto& ComboData : Combos)
	{
		if (!ComboData.Value.ActiveCombo)
			continue;

		ComboData.Key->Weapon->ResetHits();
		ComboData.Value.ActiveCombo->EndCombo();
		ComboData.Value.ActiveCombo = nullptr;
	}
}

FGameplayAbilitySpecHandle* UAbilityComboManager::SwitchAndAdvanceCombo(APickupActor* PickupActor, UAbilityCombo* Combo)
{
	int startFrom = Combos[PickupActor].ActiveCombo->LastComboAttackNumber++;
	Combos[PickupActor].ActiveCombo->EndCombo();
	Combos[PickupActor].ActiveCombo = Combo;
	return Combo->StartCombo(startFrom);
}
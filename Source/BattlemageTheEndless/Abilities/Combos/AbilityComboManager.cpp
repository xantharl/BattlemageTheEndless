// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilityComboManager.h"
#include "Abilities/GameplayAbility.h"
#include <BattlemageTheEndless/Abilities/AttackBaseGameplayAbility.h>

void UAbilityComboManager::AddAbilityToCombo(APickupActor* PickupActor, UAttackBaseGameplayAbility* Ability, FGameplayAbilitySpecHandle Handle)
{
	// We build combos for each pickup the first time it is equipped and assume they will not change during gameplay
	// TODO: If we want to allow for dynamic combos, we will need an event this can subscribe to for a rebuild
	if (!Combos.Contains(PickupActor))
	{
		Combos.Add(PickupActor, FPickupCombos());
	}

	// nothing to do if this ability isn't part of a combo
	if (!Ability->HasComboTag())
		return;

	// it is expected that the resulting tag will be something like Weapons.Sword.LightAttack.1
	// meaning we want the direct parent to be the BaseComboIdentifier
	FGameplayTag parentTag = Ability->GetAbilityName().GetGameplayTagParents().GetByIndex(1);
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
	// If we aren't aware of any combos for this pickup or the weapon's active ability has no combos, delegate to the weapon
	if (!Combos.Contains(PickupActor) || PickupActor->Weapon->ActiveAbility 
		&& !PickupActor->Weapon->ActiveAbility->GetDefaultObject<UAttackBaseGameplayAbility>()->HasComboTag())
	{
		DelegateToWeapon(PickupActor, AttackType);
		return;
	}

	// if the active ability does not have a combo, delegate to the weapon's input handler

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
			UE_LOG(LogTemp, Warning, TEXT("More than one combo found for attack type %s. This is invalid configuration."), *attackTypeName);
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
	if (!toActivate && combo)
		toActivate = combo->StartCombo();
	
	if (!toActivate)
		return;

	// we've only gotten this far if we're in a combo, so we can assume the ability is part of a combo
	// if there is an ongoing ability, queue the next one
	auto ongoingEffects = AbilitySystemComponent->GetActiveGameplayEffects();
	auto query = FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(FGameplayTagContainer(FGameplayTag::RequestGameplayTag("State.Combo")));
	if (ongoingEffects.GetNumGameplayEffects() > 0 && ongoingEffects.GetActiveEffects(query).Num() > 0)
	{
		NextAbilityHandle = toActivate;
		auto remainingTime = FMath::Max(AbilitySystemComponent->GetActiveEffectsDuration(query));
		FTimerDelegate TimerDelegate = FTimerDelegate::CreateUObject(this, &UAbilityComboManager::ActivateAbilityAndResetTimer, *toActivate);
		GetWorld()->GetTimerManager().SetTimer(QueuedAbilityTimer, TimerDelegate, remainingTime, false);
	}

	// otherwise immediately execute the ability
	ActivateAbilityAndResetTimer(*toActivate);
}

// TODO: Separate the logic for spell and melee
void UAbilityComboManager::DelegateToWeapon(APickupActor* PickupActor, EAttackType AttackType)
{
	TSubclassOf<UGameplayAbility> abilityClass;
	if (PickupActor->Weapon->SlotType == EquipSlot::Secondary)
		abilityClass = PickupActor->Weapon->ActiveAbility;
	else
		abilityClass = PickupActor->Weapon->GetAbilityByAttackType(AttackType);
	
	if (!abilityClass)
		return;

	AbilitySystemComponent->TryActivateAbility(
		AbilitySystemComponent->FindAbilitySpecFromClass(abilityClass)->Handle, true);
}

void UAbilityComboManager::ActivateAbilityAndResetTimer(FGameplayAbilitySpecHandle Ability)
{
	// This is for the case where we entered this function via the queued ability timer
	if (NextAbilityHandle)
		NextAbilityHandle = nullptr;

	LastActivatedAbilityHandle = &Ability;

	bool activated = AbilitySystemComponent->TryActivateAbility(Ability, true);
	auto spec = AbilitySystemComponent->FindAbilitySpecFromHandle(Ability);

	if (GEngine && spec && spec->Ability)
	{
		FString abilityName = AbilitySystemComponent->FindAbilitySpecFromHandle(Ability)->Ability->GetName();
		GEngine->AddOnScreenDebugMessage(-1, 1.50f, FColor::Yellow, FString::Printf(TEXT("Ability %s activated? %s"), 
			*abilityName, *FString(activated ? "true": "false")));
	}

	if (!activated)
		return;

	// intentionally overwrite the timer handle each time we advance the combo
	GetWorld()->GetTimerManager().SetTimer(ComboTimerHandle, [&] {EndComboHandler(); }, ComboExpiryTime, false);
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
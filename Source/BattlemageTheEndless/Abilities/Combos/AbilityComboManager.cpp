// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilityComboManager.h"
#include "Abilities/GameplayAbility.h"

void UAbilityComboManager::ParseCombos(APickupActor* PickupActor, UAbilitySystemComponent* abilitySystemComponent)
{
	// We build combos for each pickup the first time it is equipped and assume they will not change during gameplay
	// TODO: If we want to allow for dynamic combos, we will need an event this can subscribe to for a rebuild
	if (Combos.Contains(PickupActor))
	{
		return;
	}

	FPickupCombos PickupCombos;

	FGameplayTagContainer comboStateTags = FGameplayTagContainer(FGameplayTag::RequestGameplayTag(FName("State.Combo")));
	FGameplayTagContainer baseComboIdentifierTags = FGameplayTagContainer(FGameplayTag::RequestGameplayTag(FName("Weapons")));
	baseComboIdentifierTags.AddTag(FGameplayTag::RequestGameplayTag(FName("Spells")));

	//auto ownedAbilities = abilitySystemComponent->GetActivatableAbilities();

	// iterate through the abilities and create or add to a combo as needed
	for (auto& ability : PickupActor->Weapon->GrantedAbilities)
	{
		UGameplayAbility* defaultAbility = ability->GetDefaultObject<UGameplayAbility>();
		auto ownedComboStateTags = defaultAbility->AbilityTags.Filter(comboStateTags);
		
		// nothing to do if this ability isn't part of a combo
		if (ownedComboStateTags.Num() == 0)
			continue;
		else if (ownedComboStateTags.Num() > 1)
		{
			UE_LOG(LogTemp, Warning, TEXT("Ability %s has more than one State.Combo tag. This is not supported."), *ability->GetName());
			continue;
		}

		// otherwise identify this ability's BaseComboIdentifier
		auto ownedBaseComboTags = defaultAbility->AbilityTags.Filter(baseComboIdentifierTags);

		// it is expected that the resulting tag will be something like Weapons.Sword.LightAttack.1
		// meaning we want the direct parent to be the BaseComboIdentifier
		FGameplayTag parentTag = ownedBaseComboTags.First().GetGameplayTagParents().GetByIndex(1);
		UAbilityCombo* combo = nullptr;
		if (PickupCombos.Combos.Num() > 0)
		{
			UAbilityCombo** searchResult = PickupCombos.Combos.FindByPredicate([parentTag](const UAbilityCombo* combo) {return combo->BaseComboIdentifier == parentTag; });
			if (searchResult)
			{
				combo = *searchResult;
			}
		}

		// find the ability's handle in the ASC so we can reference the instance later
		auto spec = abilitySystemComponent->FindAbilitySpecFromClass(ability);
		if (!spec)
		{
			UE_LOG(LogTemp, Warning, TEXT("Ability %s not found in ASC."), *ability->GetName());
			continue;
		}
		auto handle = spec->Handle;

		if (combo)
		{			
			combo->AbilityHandles.Add(handle);
		}
		else
		{
			UAbilityCombo* newCombo = NewObject<UAbilityCombo>();
			newCombo->BaseComboIdentifier = parentTag;
			newCombo->AbilityHandles.Add(handle);
			PickupCombos.Combos.Add(newCombo);
		}
	}

}

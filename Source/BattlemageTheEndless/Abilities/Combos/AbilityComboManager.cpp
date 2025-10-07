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

	// it is expected that the resulting tag will be something like Weapons.Sword.LightAttack
	FGameplayTag parentTag = Ability->GetAbilityName();
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

FGameplayAbilitySpecHandle UAbilityComboManager::ProcessInput(APickupActor* PickupActor, EAttackType AttackType)
{
	// ignore input if there is a queued action and we are requesting the same combo
	bool activeComboContainsRequestedAttack = Combos.Contains(PickupActor) && Combos[PickupActor].ActiveCombo
		&& Combos[PickupActor].ActiveCombo->BaseComboIdentifier.GetTagName().ToString().Contains(*UEnum::GetDisplayValueAsText(AttackType).ToString());
	if (NextAbilityHandle && activeComboContainsRequestedAttack)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 1.50f, FColor::Yellow, TEXT("Ignoring input, requested action already queued"));
		}
		return FGameplayAbilitySpecHandle();
	}

	// If we aren't aware of any combos for this pickup or the weapon's active ability has no combos, delegate to the weapon
	if (!Combos.Contains(PickupActor) || PickupActor->Weapon->SelectedAbility 
		&& !PickupActor->Weapon->SelectedAbility->GetDefaultObject<UAttackBaseGameplayAbility>()->HasComboTag())
	{
		return DelegateToWeapon(PickupActor, AttackType);
	}

	// if the active ability does not have a combo, delegate to the weapon's input handler

	//FPickupCombos& pickupCombos = Combos[PickupActor];
	auto nameTag = PickupActor->Weapon->SelectedAbility->GetDefaultObject<UAttackBaseGameplayAbility>()->GetAbilityName();
	
	UAbilityCombo* combo = FindComboByTag(PickupActor, nameTag);

	//// TODO: This is kind of a hack to get spells working (since they don't have heavy/light combos)
	//if(pickupCombos.Combos.Num() == 1)
	//{
	//	combo = pickupCombos.Combos[0];
	//}
	//else
	//{
	//	FString attackTypeName = *UEnum::GetDisplayValueAsText(AttackType).ToString();
	//	// otherwise we need to find the most applicable Combo based on the AttackType and then start or advance the combo
	//	TArray<UAbilityCombo*> matches = pickupCombos.Combos.FilterByPredicate(
	//		[attackTypeName](const UAbilityCombo* combo) {
	//			return combo->BaseComboIdentifier.GetTagName().ToString().Contains(attackTypeName); });

	//	// If the current attack isn't part of any combo, delegate to the weapon's input handler
	//	if (matches.Num() == 0)
	//	{
	//		return DelegateToWeapon(PickupActor, AttackType);
	//	}
	//	else if (matches.Num() > 1)
	//	{
	//		UE_LOG(LogTemp, Warning, TEXT("More than one combo found for attack type %s. This is invalid configuration."), *attackTypeName);
	//	}
	//	combo = matches[0];
	//}

	// if we're switching combos (e.g. heavy to light) we need to replace the active combo and advance the new one
	bool isComboSwap = Combos[PickupActor].ActiveCombo && Combos[PickupActor].ActiveCombo != combo;
	FGameplayAbilitySpecHandle* toActivate;
	if (isComboSwap)
	{
		toActivate = SwitchAndAdvanceCombo(PickupActor, combo);
	}
	else
	{
		toActivate = Combos[PickupActor].ActiveCombo ? combo->AdvanceCombo() : combo->StartCombo();
	}

	Combos[PickupActor].ActiveCombo = combo;
	// if we found a next ability, activate it
	if (!toActivate && combo)
		toActivate = combo->StartCombo();
	
	if (!toActivate)
		return FGameplayAbilitySpecHandle();

	// we've only gotten this far if we're in a combo, so we can assume the ability is part of a combo
	// if there is an ongoing ability, queue the next one and subscribe to the end event
	if (LastActivatedAbilityHandle.IsValid())
	{
		auto lastAbility = AbilitySystemComponent->FindAbilitySpecFromHandle(LastActivatedAbilityHandle);
		TArray<TObjectPtr<UGameplayAbility>> instances = lastAbility->Ability->GetReplicationPolicy() == EGameplayAbilityReplicationPolicy::ReplicateNo
			? lastAbility->NonReplicatedInstances : lastAbility->ReplicatedInstances;

		// check for current ability should kill active ability case
		auto toActivateSpec = AbilitySystemComponent->FindAbilitySpecFromHandle(*toActivate);
		bool willCancelPrevious = Cast<UAttackBaseGameplayAbility>(toActivateSpec->Ability)->WillCancelAbility(lastAbility);

		// TODO: We are assuming only one instance of the ability is active at a time for now
		// Don't buffer if the next ability will cancel the current one
		if (!willCancelPrevious && instances.Num() > 0 && instances[0]->IsActive())
		{
			NextAbilityHandle = toActivate;
			instances[0]->OnGameplayAbilityEnded.AddLambda([this, toActivateSpec](UGameplayAbility* ability) {
				ActivateAbilityAndResetTimer(*toActivateSpec);
			});
		}
		else
		{
			ActivateAbilityAndResetTimer(*toActivateSpec);
		}
	}
	else
	{
		ActivateAbilityAndResetTimer(*AbilitySystemComponent->FindAbilitySpecFromHandle(*toActivate));
	}

	return *toActivate;
}

// TODO: Separate the logic for spell and melee
FGameplayAbilitySpecHandle UAbilityComboManager::DelegateToWeapon(APickupActor* PickupActor, EAttackType AttackType)
{
	TSubclassOf<UGameplayAbility> abilityClass;
	if (PickupActor->Weapon->SlotType == EquipSlot::Secondary)
		abilityClass = PickupActor->Weapon->SelectedAbility;
	else
		abilityClass = PickupActor->Weapon->GetAbilityByAttackType(AttackType);
	
	if (!abilityClass)
		return FGameplayAbilitySpecHandle();

	auto spec = AbilitySystemComponent->FindAbilitySpecFromClass(abilityClass);
	if (spec && CheckCooldownAndTryActivate(*spec))
		return spec->Handle;

	return FGameplayAbilitySpecHandle();
}

UAbilityCombo* UAbilityComboManager::FindComboByTag(APickupActor* PickupActor, const FGameplayTag& ComboTag)
{
	TArray<UAbilityCombo*> matches = Combos[PickupActor].Combos.FilterByPredicate(
		[ComboTag](const UAbilityCombo* combo) {
			return combo->BaseComboIdentifier.MatchesTag(ComboTag); });

	// If the current attack isn't part of any combo, delegate to the weapon's input handler
	if (matches.Num() == 0)
	{
		return nullptr;
	}
	else if (matches.Num() > 1)
	{
		UE_LOG(LogTemp, Warning, TEXT("More than one combo found for tag %s. This is invalid configuration."), *ComboTag.ToString());
	}
	return matches[0];
}

void UAbilityComboManager::ActivateAbilityAndResetTimer(FGameplayAbilitySpec abilitySpec)
{
	// This is for the case where we entered this function via the queued ability timer
	if (NextAbilityHandle)
	{
		NextAbilityHandle = nullptr;

		// unsubscribe from the last ability's end event
		auto lastAbility = AbilitySystemComponent->FindAbilitySpecFromHandle(LastActivatedAbilityHandle);
		TArray<TObjectPtr<UGameplayAbility>> instances = lastAbility->Ability->GetReplicationPolicy() == EGameplayAbilityReplicationPolicy::ReplicateNo
			? lastAbility->NonReplicatedInstances : lastAbility->ReplicatedInstances;

		// TODO: We are assuming only one instance of the ability is active at a time for now
		if (instances.Num() > 0)
		{
			// This is assuming we only have one delegate
			instances[0]->OnGameplayAbilityEnded.RemoveAll(instances[0]);
		}
	}

	bool activated = CheckCooldownAndTryActivate(abilitySpec);
	if (activated)
		LastActivatedAbilityHandle = abilitySpec.Handle;

	if (GEngine && abilitySpec.Ability)
	{
		FString abilityName = abilitySpec.Ability->GetName();
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

	// Reset combo manager state
	LastActivatedAbilityHandle = FGameplayAbilitySpecHandle();
	NextAbilityHandle = nullptr;
	LastAbilityNiagaraInstance = nullptr;
	if (GetWorld()->GetTimerManager().IsTimerActive(ComboTimerHandle))
		GetWorld()->GetTimerManager().ClearTimer(ComboTimerHandle);
}

FGameplayAbilityActorInfo UAbilityComboManager::GetOwnerActorInfo()
{
	if (_ownerActorInfo.AvatarActor == nullptr)
	{
		auto owner = AbilitySystemComponent->GetOwnerActor();
		_ownerActorInfo.InitFromActor(owner, owner, AbilitySystemComponent);
	}
	return _ownerActorInfo;
}

void UAbilityComboManager::OnAbilityFailed(const UGameplayAbility* ability, const FGameplayTagContainer& reason)
{
	// if there's a non-cooldown failure, we want to know why
	if(GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 1.50f, FColor::Red,
			FString::Printf(TEXT("Ability %s failed because %s"), *ability->GetName(), *reason.First().ToString()));
	}
}

FGameplayAbilitySpecHandle* UAbilityComboManager::SwitchAndAdvanceCombo(APickupActor* PickupActor, UAbilityCombo* Combo)
{
	int startFrom = Combos[PickupActor].ActiveCombo->LastComboAttackNumber++;
	Combos[PickupActor].ActiveCombo->EndCombo();
	Combos[PickupActor].ActiveCombo = Combo;
	return Combo->StartCombo(startFrom);
}

bool UAbilityComboManager::CheckCooldownAndTryActivate(FGameplayAbilitySpec abilitySpec)
{
	// only activate if we're not on CD to avoid spamming the ASC
	const FGameplayAbilityActorInfo& actorInfo = GetOwnerActorInfo();
	if (abilitySpec.Ability->CheckCooldown(abilitySpec.Handle, &actorInfo))
		return AbilitySystemComponent->TryActivateAbility(abilitySpec.Handle, true);

	return false;
}
// Fill out your copyright notice in the Description page of Project Settings.


#include "ComboManagerComponent.h"

// Sets default values for this component's properties
UComboManagerComponent::UComboManagerComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UComboManagerComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}


// Called every frame
void UComboManagerComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}


void UComboManagerComponent::AddAbilityToCombo(APickupActor* PickupActor, UGA_WithEffectsBase* Ability, FGameplayAbilitySpecHandle Handle)
{
	// We build combos for each pickup the first time it is equipped and assume they will not change during gameplay
	if (!Combos.Contains(PickupActor))
	{
		Combos.Add(PickupActor, FPickupCombos());
	}

	// nothing to do if this ability isn't part of a combo
	if (!Ability->HasComboTag())
		return;

	// it is expected that the resulting tag will be something like Weapons.Sword.LightAttack
	FGameplayTag parentTag = Ability->GetAbilityIdentifierTag();
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

FGameplayAbilitySpecHandle UComboManagerComponent::ProcessInput_Legacy(APickupActor* PickupActor, EAttackType AttackType)
{
	// ignore input if there is a queued action and we are requesting the same combo
	bool activeComboContainsRequestedAttack = Combos.Contains(PickupActor) && Combos[PickupActor].ActiveCombo
		&& Combos[PickupActor].ActiveCombo->BaseComboIdentifier.GetTagName().ToString().Contains(*UEnum::GetDisplayValueAsText(AttackType).ToString());
	if (NextAbilityHandle && activeComboContainsRequestedAttack)
	{
		/*if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 1.50f, FColor::Yellow, TEXT("Ignoring input, requested action already queued"));
		}*/
		return FGameplayAbilitySpecHandle();
	}

	// If we aren't aware of any combos for this pickup or the weapon's active ability has no combos, delegate to the weapon
	if (!Combos.Contains(PickupActor) || PickupActor->Weapon->SelectedAbility 
		&& !PickupActor->Weapon->SelectedAbility->GetDefaultObject<UAttackBaseGameplayAbility>()->HasComboTag())
	{
		return DelegateToWeapon(PickupActor, AttackType);
	}

	// if the active ability does not have a combo, delegate to the weapon's input handler

	FGameplayTag nameTag;
	if (PickupActor->PickupType == EPickupType::Weapon)
	{
		nameTag = (AttackType == EAttackType::Light
			? (FGameplayTag::RequestGameplayTag(FName("Weapon.AttackType.Light")))
			: (FGameplayTag::RequestGameplayTag(FName("Weapon.AttackType.Heavy"))));
	}
	// this handler is expected to only be hit for weapons now but keeping the fallback to be safe
	else 
		nameTag = PickupActor->Weapon->SelectedAbility->GetDefaultObject<UAttackBaseGameplayAbility>()->GetAbilityIdentifierTag();
	
	UAbilityCombo* combo = FindComboByTag(PickupActor, nameTag);

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
	if (LastActivatedAbilityClass)
	{
		auto lastAbility = AbilitySystemComponent->FindAbilitySpecFromClass(LastActivatedAbilityClass);
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
		auto spec = AbilitySystemComponent->FindAbilitySpecFromHandle(*toActivate);
		if (!spec)
		{
			UE_LOG(LogTemp, Warning, TEXT("No ability spec found for handle in combo activation"));
			return FGameplayAbilitySpecHandle();
		}
		ActivateAbilityAndResetTimer(*AbilitySystemComponent->FindAbilitySpecFromHandle(*toActivate));
	}

	return *toActivate;
}

FGameplayAbilitySpecHandle UComboManagerComponent::ProcessInput(APickupActor* PickupActor, EAttackType AttackType)
{
	// Ignore input if there is a queued action
	if (NextAbilityHandle)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 1.50f, FColor::Yellow, TEXT("Ignoring input, requested action already queued"));
		}
		return FGameplayAbilitySpecHandle();
	}	
	
	// Start building search tags based on input
	FGameplayTagContainer OwnedTags = FGameplayTagContainer(AttackType == EAttackType::Light
			? FGameplayTag::RequestGameplayTag(FName("Weapon.AttackType.Light"))
			: FGameplayTag::RequestGameplayTag(FName("Weapon.AttackType.Heavy")));
	
	// If we are currently in a combo, look for the next ability in the combo, otherwise look for the first
	if (const FTimerManager& TimerManager = GetWorld()->GetTimerManager(); TimerManager.IsTimerActive(ComboTimerHandle))
	{
		if (!LastActivatedAbilityClass)
			OwnedTags.AddTagFast(FGameplayTag::RequestGameplayTag("State.Combo.1"));
		else
		{
			FGameplayTag ComboBaseTag = FGameplayTag::RequestGameplayTag("State.Combo");
			FGameplayTagContainer Matches = LastActivatedAbilityClass->GetDefaultObject<UGameplayAbility>()->AbilityTags.Filter(FGameplayTagContainer(ComboBaseTag));
			if (Matches.Num() > 0)
			{
				int ComboStage = 1;
				FString LastComboTagStr = Matches.First().ToString();
				// extract the numeric suffix, this assumes we will never have more than 10 stages in a combo
				int32 LastStage = FCString::Atoi(*LastComboTagStr.Right(1));
				ComboStage = LastStage + 1;
				FString NewComboTagStr = FString::Printf(TEXT("State.Combo.%d"), ComboStage);
				OwnedTags.AddTagFast(FGameplayTag::RequestGameplayTag(*NewComboTagStr));
			}
			else
			{
				// fallback to first in combo
				OwnedTags.AddTagFast(FGameplayTag::RequestGameplayTag("State.Combo.1"));
			}
		}
	}
	else		
		OwnedTags.AddTagFast(FGameplayTag::RequestGameplayTag("State.Combo.1"));
	
	// Interrogate the owner to see if we have any active abilities which will modify the outcome
	// NOTE: We technically could just try to activate the ability and let GAS handle the failure, but this way we can avoid unnecessary network traffic
	auto IsSprinting = AbilitySystemComponent->HasMatchingGameplayTag(FGameplayTag::RequestGameplayTag(FName("Movement.Sprint")));
	auto IsFalling = false;
	const auto Owner = GetOwner();
	if (!Owner)
	{
		UE_LOG(LogTemp, Error, TEXT("ComboManagerComponent has no owner"));
		return FGameplayAbilitySpecHandle();
	}
	
	if (const auto Character = Cast<ACharacter>(Owner); Character->GetCharacterMovement())
	{
		IsFalling = Character->GetCharacterMovement()->IsFalling();
	}
	
	// Determine the best ability to use based on current state
	FGameplayTagContainer RequiredTags;
	if (IsFalling)
		RequiredTags.AddTagFast(FGameplayTag::RequestGameplayTag(FName("Movement.Falling")));
	else if (IsSprinting)
		RequiredTags.AddTagFast(FGameplayTag::RequestGameplayTag(FName("Movement.Sprint")));
	
	auto Result = PickupActor->Weapon->GrantedAbilities
		.FindByPredicate([OwnedTags, RequiredTags](TSubclassOf<UGameplayAbility> abilityClass)
	    {
			const auto AbilityDefault = abilityClass->GetDefaultObject<UGA_WithEffectsBase>();
			return AbilityDefault->AbilityTags.HasAll(OwnedTags)
				&& (RequiredTags.IsEmpty() || AbilityDefault->GetActivationRequiredTags().HasAll(RequiredTags));
	    });
	
	// If no ability found with state tags, try again with first stage in combo
	if (!Result)
	{
		// TODO: Find a more efficient way to do this
		OwnedTags.RemoveTags(OwnedTags.Filter(FGameplayTagContainer(FGameplayTag::RequestGameplayTag("State.Combo"))));
		OwnedTags.AddTagFast(FGameplayTag::RequestGameplayTag("State.Combo.1"));
	
		Result = PickupActor->Weapon->GrantedAbilities.FindByPredicate([OwnedTags](TSubclassOf<UGameplayAbility> abilityClass)
		{
		   return abilityClass->GetDefaultObject<UGameplayAbility>()->AbilityTags.HasAll(OwnedTags);
		});
	}
	
	// If we still got nothing, ask the weapon to decide
	if (!Result)
		return DelegateToWeapon(PickupActor, AttackType);
	
	FGameplayAbilitySpec* ToActivate = AbilitySystemComponent->FindAbilitySpecFromClass(Result->Get());
	
	UE_LOG( LogTemp, Log, TEXT("ComboManagerComponent::ProcessInput activating ability %s"), *Result->Get()->GetName());
	
	// If the last ability is still active, queue the next one
	if (LastActivatedAbilityClass)
	{
		const auto LastActivatedSpec = AbilitySystemComponent->FindAbilitySpecFromClass(LastActivatedAbilityClass);
		if (const auto PrimaryInstance = LastActivatedSpec->GetPrimaryInstance(); 
			PrimaryInstance && PrimaryInstance->IsActive())
		{			
			NextAbilityHandle = &ToActivate->Handle;
			LastActivatedSpec->GetPrimaryInstance()->OnGameplayAbilityEnded.AddLambda([this, ToActivate](UGameplayAbility* _) {
				ActivateAbilityAndResetTimer(*ToActivate);
			});
		}
	}
	// Otherwise activate immediately
	ActivateAbilityAndResetTimer(*ToActivate);
	return ToActivate->Handle;
}

// TODO: Separate the logic for spell and melee
FGameplayAbilitySpecHandle UComboManagerComponent::DelegateToWeapon(APickupActor* PickupActor, EAttackType AttackType)
{
	if (!PickupActor)
	{
		UE_LOG(LogTemp, Warning, TEXT("No PickupActor provided to DelegateToWeapon"));
		return FGameplayAbilitySpecHandle();
	}
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

UAbilityCombo* UComboManagerComponent::FindComboByTag(APickupActor* PickupActor, const FGameplayTag& ComboTag)
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

void UComboManagerComponent::ActivateAbilityAndResetTimer(FGameplayAbilitySpec abilitySpec)
{
	// This is for the case where we entered this function via the queued ability timer
	if (NextAbilityHandle)
	{
		NextAbilityHandle = nullptr;

		// unsubscribe from the last ability's end event
		auto lastAbility = AbilitySystemComponent->FindAbilitySpecFromClass(LastActivatedAbilityClass);
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
		LastActivatedAbilityClass = abilitySpec.Ability->GetClass();

	//if (GEngine && abilitySpec.Ability)
	//{
	//	FString abilityName = abilitySpec.Ability->GetName();
	//	GEngine->AddOnScreenDebugMessage(-1, 1.50f, FColor::Yellow, FString::Printf(TEXT("Ability %s activated? %s"), 
	//		*abilityName, *FString(activated ? "true": "false")));
	//}

	if (!activated)
		return;

	// intentionally overwrite the timer handle each time we advance the combo
	float timerDuration = ComboExpiryTime;

	// account for montage duration so we start the combo expiry after the attack is over
	auto attackBase = Cast<UAttackBaseGameplayAbility>(abilitySpec.Ability);
	if (attackBase && attackBase->FireAnimation)
		timerDuration += attackBase->FireAnimation->GetPlayLength();

	UE_LOG( LogTemp, Log, TEXT("ComboManagerComponent::ActivateAbilityAndResetTimer setting combo timer for %f seconds"), timerDuration);
	
	GetWorld()->GetTimerManager().SetTimer(ComboTimerHandle, [&] {EndComboHandler(); }, timerDuration, false);
}

void UComboManagerComponent::EndComboHandler() 
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
	LastActivatedAbilityClass = nullptr;
	NextAbilityHandle = nullptr;
	LastAbilityNiagaraInstance = nullptr;
	if (GetWorld()->GetTimerManager().IsTimerActive(ComboTimerHandle))
		GetWorld()->GetTimerManager().ClearTimer(ComboTimerHandle);
}

FGameplayAbilityActorInfo UComboManagerComponent::GetOwnerActorInfo()
{
	if (_ownerActorInfo.AvatarActor == nullptr)
	{
		auto owner = AbilitySystemComponent->GetOwnerActor();
		_ownerActorInfo.InitFromActor(owner, owner, AbilitySystemComponent);
	}
	return _ownerActorInfo;
}

void UComboManagerComponent::OnAbilityFailed(const UGameplayAbility* ability, const FGameplayTagContainer& reason)
{
	// if there's a non-cooldown failure, we want to know why
	if(GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 1.50f, FColor::Red,
			FString::Printf(TEXT("Ability %s failed because %s"), *ability->GetName(), *reason.First().ToString()));
	}
}

FGameplayAbilitySpecHandle* UComboManagerComponent::SwitchAndAdvanceCombo(APickupActor* PickupActor, UAbilityCombo* Combo)
{
	int startFrom = Combos[PickupActor].ActiveCombo->LastComboAttackNumber++;
	Combos[PickupActor].ActiveCombo->EndCombo();
	Combos[PickupActor].ActiveCombo = Combo;
	return Combo->StartCombo(startFrom);
}

bool UComboManagerComponent::CheckCooldownAndTryActivate(FGameplayAbilitySpec abilitySpec)
{
	// only activate if we're not on CD to avoid spamming the ASC
	const FGameplayAbilityActorInfo& actorInfo = GetOwnerActorInfo();
	if (abilitySpec.Ability->CheckCooldown(abilitySpec.Handle, &actorInfo))
		return AbilitySystemComponent->TryActivateAbility(abilitySpec.Handle, true);

	return false;
}
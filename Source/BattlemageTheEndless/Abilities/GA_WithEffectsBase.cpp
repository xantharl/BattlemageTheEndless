// Fill out your copyright notice in the Description page of Project Settings.


#include "GA_WithEffectsBase.h"

#include "BaseAttributeSet.h"
#include "GEComp_DefaultDamage.h"

FGameplayTag UGA_WithEffectsBase::GetAbilityIdentifierTag()
{
	FGameplayTagContainer baseComboIdentifierTags = FGameplayTagContainer(FGameplayTag::RequestGameplayTag(FName("Weapon")));
	baseComboIdentifierTags.AddTag(FGameplayTag::RequestGameplayTag(FName("Spells")));

	// otherwise identify this ability's BaseComboIdentifier
	auto Tags = AbilityTags.Filter(baseComboIdentifierTags);
	auto TypesTag = FGameplayTag::RequestGameplayTag(FName("Spells.Types"));
	if (Tags.Num() > 0)
	{
		for (FGameplayTag Tag : Tags)
		{
			if (!Tag.MatchesTag(TypesTag))
				return Tag;
		}
		
		return Tags.Last();
	}
	
	UE_LOG(LogTemp, Error, TEXT("Ability %s has no Weapons or Spells tag, returning empty tag"), *GetName());
	return FGameplayTag();
}

FName UGA_WithEffectsBase::GetAbilityName()
{
	FGameplayTag IdentifierTag = GetAbilityIdentifierTag();
	if (!IdentifierTag.IsValid())
		return FName(GetName());
	
	// Handle combo tags by stripping numeric suffixes
	while (IdentifierTag.GetTagLeafName().ToString().IsNumeric())
	{
		IdentifierTag = IdentifierTag.RequestDirectParent();
	}
	
	return IdentifierTag.GetTagLeafName();
}

void UGA_WithEffectsBase::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	// Default impl assumes applying effects to self, override if needed
	if (ActiveEffectHandles.IsEmpty() && EffectsToApply.Num() > 0)
	{		
		ActiveEffectHandles = ApplyEffects(ActorInfo->OwnerActor.Get(), ActorInfo->AbilitySystemComponent.Get(), ActorInfo->OwnerActor.Get(), ActorInfo->AvatarActor.Get());
	}
}

TArray<FActiveGameplayEffectHandle> UGA_WithEffectsBase::ApplyEffects(const AActor* Target, UAbilitySystemComponent* TargetAsc, AActor* Instigator, AActor* EffectCauser) const
{	
	TArray<FActiveGameplayEffectHandle> InnerEffectHandles;
	for (TSubclassOf<UGameplayEffect> effect : EffectsToApply)
	{
		FGameplayEffectContextHandle context = TargetAsc->MakeEffectContext();
		if (Instigator)
		{
			context.AddSourceObject(Instigator);
			context.AddInstigator(Instigator, EffectCauser ? EffectCauser : Instigator);
			context.AddActors({ Instigator });
		}

		FGameplayEffectSpecHandle specHandle = TargetAsc->MakeOutgoingSpec(effect, 1.f, context);
		if (!specHandle.IsValid())
			continue;

		HandleSetByCaller(effect, specHandle, EffectCauser, TargetAsc);

		const FActiveGameplayEffectHandle handle = TargetAsc->ApplyGameplayEffectSpecToSelf(*specHandle.Data.Get());
		InnerEffectHandles.Add(handle);
	}
	
	return InnerEffectHandles;
}

void UGA_WithEffectsBase::HandleSetByCaller(TSubclassOf<UGameplayEffect> effect, FGameplayEffectSpecHandle specHandle, AActor* effectCauser, UAbilitySystemComponent* targetAsc) const
{
	if (!targetAsc) 
		return;

	// TODO: Support DamageModifiers from the attacker
	
	// currently we only care about physical damage for resistance/modifier purposes
	// We're also assuming all weapons are physical damage, which is true at least for now
	FGameplayTagContainer WeaponAttackTags = FGameplayTagContainer(FGameplayTag::RequestGameplayTag("Weapon.AttackType"));
	WeaponAttackTags.AddTag(FGameplayTag::RequestGameplayTag("Weapon.Attack"));
	if (!this->AbilityTags.HasAny(WeaponAttackTags))
		return;

	auto TargetAttributeSet = Cast<UBaseAttributeSet>(targetAsc->GetAttributeSet(UBaseAttributeSet::StaticClass()));
	if (!TargetAttributeSet)
		return;
	
	auto DefaultValueComponent = Cast<UGameplayEffect>(effect->GetDefaultObject())->FindComponent(UGEComp_DefaultDamage::StaticClass());
	float EffectiveValue;
	if (!DefaultValueComponent)
		return;
	{
		EffectiveValue = Cast<UGEComp_DefaultDamage>(DefaultValueComponent)->DefaultValue;
	}
	
	if (FMath::Abs(TargetAttributeSet->GetDamageModifierPhysical_Inbound() - 1.f) > KINDA_SMALL_NUMBER)
	{		
		EffectiveValue *= TargetAttributeSet->GetDamageModifierPhysical_Inbound();
	}
	
	specHandle.Data->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag("Data.Damage.Physical"), EffectiveValue);
}


void UGA_WithEffectsBase::CancelAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateCancelAbility)
{
	ResetTimerAndClearEffects(ActorInfo, true);
	// Super calls end
	OnAbilityEnded.Broadcast(true);
	Super::CancelAbility(Handle, ActorInfo, ActivationInfo, bReplicateCancelAbility);
}

void UGA_WithEffectsBase::ResetTimerAndClearEffects(const FGameplayAbilityActorInfo* ActorInfo, bool wasCancelled)
{
	// if we have a timer running to end the ability, clear it
	UWorld* const world = GetWorld();
	if (world && world->GetTimerManager().IsTimerActive(EndTimerHandle))
	{
		world->GetTimerManager().ClearTimer(EndTimerHandle);
	}

	// remove any active effects we applied to OwnerActor
	if (ActorInfo && ActorInfo->AbilitySystemComponent.IsValid())
	{
		for (auto handle : ActiveEffectHandles)
		{
			ActorInfo->AbilitySystemComponent->RemoveActiveGameplayEffect(handle);
		}
		ActiveEffectHandles.Empty();
	}
}
void UGA_WithEffectsBase::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (CooldownGameplayEffectClass)
		CommitAbilityCooldown(Handle, ActorInfo, ActivationInfo, false);

	OnAbilityEnded.Broadcast(false);
	ResetTimerAndClearEffects(ActorInfo);
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGA_WithEffectsBase::EndSelf()
{
	if (IsActive())
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	// else if (GEngine)
	// 	GEngine->AddOnScreenDebugMessage(-1, 1.5f, FColor::Blue, FString::Printf(TEXT("Ability %s is not active, cannot end it"), *GetName()));
}

void UGA_WithEffectsBase::EndAbilityByTimeout (const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	// if (GEngine)
	// 	GEngine->AddOnScreenDebugMessage(-1, 1.5f, FColor::Red, FString::Printf(TEXT("Ending ability %s due to timeout"), *GetName()));
	EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

bool UGA_WithEffectsBase::WillCancelAbility(FGameplayAbilitySpec* OtherAbility)
{
	if (CancelAbilitiesWithTag.Num() == 0)
		return false;

	return OtherAbility->Ability->AbilityTags.HasAny(CancelAbilitiesWithTag);
}

TArray<TObjectPtr<UGA_WithEffectsBase>> UGA_WithEffectsBase::GetAbilityActiveInstances(FGameplayAbilitySpec* spec)
{
	TArray<TObjectPtr<UGA_WithEffectsBase>> returnVal;
	auto instances = GetReplicationPolicy() == EGameplayAbilityReplicationPolicy::ReplicateNo
		? spec->NonReplicatedInstances : spec->ReplicatedInstances;

	for (auto instance : instances)
	{
		returnVal.Add(Cast<UGA_WithEffectsBase>(instance));
	}

	return returnVal;
}

bool UGA_WithEffectsBase::HasComboTag()
{
	return GetComboTags().Num() > 0;
}

bool UGA_WithEffectsBase::IsFirstInCombo()
{
	auto comboTags = GetComboTags();
	return comboTags.Num() > 0 && comboTags.First().ToString().EndsWith(".1");
}

FGameplayTagContainer UGA_WithEffectsBase::GetComboTags()
{
	FGameplayTagContainer comboStateTags = FGameplayTagContainer(FGameplayTag::RequestGameplayTag(FName("State.Combo")));
	auto comboTags = AbilityTags.Filter(comboStateTags);

	if (comboTags.Num() > 1)
	{
		UE_LOG(LogTemp, Error, TEXT("Ability %s has more than one State.Combo tag. This is not supported."), *GetName());
	}

	return comboTags;
}

TArray<FAbilityTriggerData> UGA_WithEffectsBase::GetAbilityTriggers(EGameplayAbilityTriggerSource::Type TriggerSource)
{
	return AbilityTriggers.FilterByPredicate([TriggerSource](const FAbilityTriggerData Trigger)
	{
		return Trigger.TriggerSource == TriggerSource;
	});
}

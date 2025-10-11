// Fill out your copyright notice in the Description page of Project Settings.


#include "GA_WithEffectsBase.h"

FGameplayTag UGA_WithEffectsBase::GetAbilityName()
{
	FGameplayTagContainer baseComboIdentifierTags = FGameplayTagContainer(FGameplayTag::RequestGameplayTag(FName("Weapons")));
	baseComboIdentifierTags.AddTag(FGameplayTag::RequestGameplayTag(FName("Spells")));

	// otherwise identify this ability's BaseComboIdentifier
	auto tags = AbilityTags.Filter(baseComboIdentifierTags);
	if (tags.Num() > 0)
	{
		return tags.GetByIndex(0);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Ability %s has no Weapons or Spells tag, returning empty tag"), *GetName());
		return FGameplayTag();
	}
}

void UGA_WithEffectsBase::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	// Default impl assumes applying effects to self, override if needed
	if (EffectsToApply.Num() > 0)
	{
		ApplyEffects(ActorInfo->OwnerActor.Get(), ActorInfo->AbilitySystemComponent.Get(), ActorInfo->OwnerActor.Get(), ActorInfo->AvatarActor.Get());
	}
}

void UGA_WithEffectsBase::ApplyEffects(AActor* target, UAbilitySystemComponent* targetAsc, AActor* instigator, AActor* effectCauser)
{
	for (TSubclassOf<UGameplayEffect> effect : EffectsToApply)
	{
		FGameplayEffectContextHandle context = targetAsc->MakeEffectContext();
		if (instigator)
		{
			context.AddSourceObject(instigator);
			context.AddInstigator(instigator, effectCauser ? effectCauser : instigator);
			context.AddActors({ instigator });
		}

		FGameplayEffectSpecHandle specHandle = targetAsc->MakeOutgoingSpec(effect, 1.f, context);
		if (!specHandle.IsValid())
			continue;

		HandleSetByCaller(effect, specHandle, effectCauser);

		auto handle = targetAsc->ApplyGameplayEffectSpecToSelf(*specHandle.Data.Get());
		ActiveEffectHandles.Add(handle);
	}
}

void UGA_WithEffectsBase::HandleSetByCaller(TSubclassOf<UGameplayEffect> effect, FGameplayEffectSpecHandle specHandle, AActor* effectCauser)
{
	// intentionally empty, meant to be overridden by implementers
}


void UGA_WithEffectsBase::CancelAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateCancelAbility)
{
	ResetTimerAndClearEffects(ActorInfo, true);
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

	ResetTimerAndClearEffects(ActorInfo);
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGA_WithEffectsBase::EndSelf()
{
	if (IsActive())
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	else if (GEngine)
		GEngine->AddOnScreenDebugMessage(-1, 1.5f, FColor::Blue, FString::Printf(TEXT("Ability %s is not active, cannot end it"), *GetName()));
}

void UGA_WithEffectsBase::EndAbilityByTimeout (const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (GEngine)
		GEngine->AddOnScreenDebugMessage(-1, 1.5f, FColor::Red, FString::Printf(TEXT("Ending ability %s due to timeout"), *GetName()));
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
// Fill out your copyright notice in the Description page of Project Settings.

#include "AttackBaseGameplayAbility.h"

FGameplayTag UAttackBaseGameplayAbility::GetAbilityName()
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

bool UAttackBaseGameplayAbility::HasComboTag()
{
	return GetComboTags().Num() > 0;
}

bool UAttackBaseGameplayAbility::IsFirstInCombo()
{	
	auto comboTags = GetComboTags();
	return comboTags.Num() > 0 && comboTags.First().ToString().EndsWith(".1");
}

FGameplayTagContainer UAttackBaseGameplayAbility::GetComboTags()
{
	FGameplayTagContainer comboStateTags = FGameplayTagContainer(FGameplayTag::RequestGameplayTag(FName("State.Combo")));
	auto comboTags = AbilityTags.Filter(comboStateTags);

	if (comboTags.Num() > 1)
	{
		UE_LOG(LogTemp, Error, TEXT("Ability %s has more than one State.Combo tag. This is not supported."), *GetName());
	}

	return comboTags;
}

void UAttackBaseGameplayAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	UWorld* const world = GetWorld();
	_bIsCharged = false;
	auto character = Cast<ACharacter>(ActorInfo->OwnerActor);
	if (!world || !character)
	{
		return;
	}

	// Activation time is only used to determine if the ability is charged, so if this isn't a charge ability we don't need to set it
	if (ChargeDuration > 0.001f) {
		ActivationTime = duration_cast<milliseconds>(system_clock::now().time_since_epoch());
	}

	// Try and play a firing animation if specified
	auto animInstance = character->GetMesh()->GetAnimInstance();
	float montageDuration = 0.f;
	if (FireAnimation && animInstance && ChargeDuration <= 0.001f)
	{
		// if it's not a charge ability, play the fire animation and possibly end the ability on callback (it will check for active effects)
		CreateAndDispatchMontageTask();

		// TODO: account for rate parameter when added
		montageDuration = FireAnimation->GetPlayLength();
	}
	else if (ChargeDuration > 0.001f && ChargeAnimation && animInstance)
	{
		// No need for anything fancy, just play the charge animation
		animInstance->Montage_Play(ChargeAnimation);
	};

	CommitAbility(Handle, ActorInfo, ActivationInfo);

	// If we've got a charge time, simply exit and let handler worry about ending the ability
	if (ChargeDuration >= 0.001f)
	{
		return;
	}

	// Apply effects to the character, these will in turn spawn any configured cues (Particles and/or sound)
	if(HitType == HitType::Self)
	{
		ApplyEffects(character, ActorInfo->AbilitySystemComponent.Get(), character, ActorInfo->AvatarActor.Get());

		// If any duration effects were applied, don't set an end timer, let the effect tasks handle the end
		auto durationEffects = EffectsToApply.FilterByPredicate([](TSubclassOf<UGameplayEffect> effect) {
			return effect.GetDefaultObject()->DurationPolicy == EGameplayEffectDurationType::HasDuration;
		});
		if (durationEffects.Num() > 0)
			return;
	}

	// If we have no montage, no effects, and no charge time, end the ability immediately
	if (montageDuration < 0.001f)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	// if the montage has the longer duration, set a timer to end the ability after that duration
	FTimerDelegate TimerDelegate = FTimerDelegate::CreateUObject(this, &UAttackBaseGameplayAbility::EndAbility, Handle, ActorInfo, ActivationInfo, true, true);
	world->GetTimerManager().SetTimer(EndTimerHandle, TimerDelegate, montageDuration, false);

	// set a timeout for safety (in case we've neglected to end this gracefully)
	if (IsActive())
	{
		FTimerDelegate TimeoutDelegate = FTimerDelegate::CreateUObject(this, &UAttackBaseGameplayAbility::EndAbilityByTimeout, Handle, ActorInfo, ActivationInfo, true, true);
		world->GetTimerManager().SetTimer(TimeoutTimerHandle, TimeoutDelegate, montageDuration + 1.f, false);
	}
}

void UAttackBaseGameplayAbility::CreateAndDispatchMontageTask()
{
	// TODO: Add a montage rate parameter to the ability
	auto task = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, FireAnimation,
		1.0f, NAME_None, true, 1.0f, 0.f, true);
	task->OnCompleted.AddDynamic(this, &UAttackBaseGameplayAbility::OnMontageCompleted);
	task->OnInterrupted.AddDynamic(this, &UAttackBaseGameplayAbility::OnMontageCancelled);
	task->OnCancelled.AddDynamic(this, &UAttackBaseGameplayAbility::OnMontageCancelled);
	task->ReadyForActivation();
}

void UAttackBaseGameplayAbility::ApplyEffects(AActor* target, UAbilitySystemComponent* targetAsc, AActor* instigator, AActor* effectCauser)
{
	if (ChainSystem && instigator)
	{
		// We don't need to keep the reference, UE will handle disposing when it is destroyed
		auto chainEffectActor = instigator->GetWorld()->SpawnActor<AHitScanChainEffect>(AHitScanChainEffect::StaticClass(), instigator->GetActorLocation(), FRotator::ZeroRotator);
		chainEffectActor->Init(instigator, target, ChainSystem);
	}

	for (TSubclassOf<UGameplayEffect> effect : EffectsToApply)
	{
		FGameplayEffectContextHandle context = targetAsc->MakeEffectContext();
		if(instigator)
		{
			context.AddSourceObject(instigator);
			context.AddInstigator(instigator, effectCauser ? effectCauser : instigator);
			context.AddActors({ instigator });
		}

		FGameplayEffectSpecHandle specHandle = targetAsc->MakeOutgoingSpec(effect, 1.f, context);
		if (specHandle.IsValid())
		{
			// This sets the damage manually for a Set By Caller type effect
			ABattlemageTheEndlessProjectile* projectile = Cast<ABattlemageTheEndlessProjectile>(effectCauser);
			if (projectile && FMath::Abs(projectile->EffectiveDamage) > 0.0001f)
			{
				specHandle.Data.Get()->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(FName("Spells.Charge.Damage")), projectile->EffectiveDamage);
				if (GEngine)
					GEngine->AddOnScreenDebugMessage(-1, 1.5f, FColor::Yellow, 
						FString::Printf(TEXT("%s hit for %f"), *effect->GetName(), projectile->EffectiveDamage));
			}

			auto handle = targetAsc->ApplyGameplayEffectSpecToSelf(*specHandle.Data.Get());
			ActiveEffectHandles.Add(handle);

			// TODO: Figure out what to do with infinite effects (keep alive?)
			auto durationPolicy = effect.GetDefaultObject()->DurationPolicy;
			if (durationPolicy == EGameplayEffectDurationType::HasDuration)
			{
				auto task = UAbilityTask_WaitGameplayEffectRemoved::WaitForGameplayEffectRemoved(this, handle);
				task->OnRemoved.AddDynamic(this, &UAttackBaseGameplayAbility::OnEffectRemoved);
				task->ReadyForActivation();
			}
			else if (durationPolicy == EGameplayEffectDurationType::Instant)
			{
				// if the effect is instant, remove it immediately
				targetAsc->RemoveActiveGameplayEffect(handle);
				ActiveEffectHandles.Remove(handle);
			}
		}
	}
}

void UAttackBaseGameplayAbility::CancelAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateCancelAbility)
{
	ResetTimerAndClearEffects(ActorInfo, true);
	Super::CancelAbility(Handle, ActorInfo, ActivationInfo, bReplicateCancelAbility);
}

void UAttackBaseGameplayAbility::ResetTimerAndClearEffects(const FGameplayAbilityActorInfo* ActorInfo, bool wasCancelled)
{
	// if we have a timer running to end the ability, clear it
	UWorld* const world = GetWorld();
	if (world && world->GetTimerManager().IsTimerActive(EndTimerHandle))
	{
		world->GetTimerManager().ClearTimer(EndTimerHandle);
	}
}
void UAttackBaseGameplayAbility::EndSelf()
{
	if (IsActive())
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	else if (GEngine)
		GEngine->AddOnScreenDebugMessage(-1, 1.5f, FColor::Blue, FString::Printf(TEXT("Ability %s is not active, cannot end it"), *GetName()));
}

void UAttackBaseGameplayAbility::EndAbilityByTimeout(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (GEngine)
		GEngine->AddOnScreenDebugMessage(-1, 1.5f, FColor::Red, FString::Printf(TEXT("Ending ability %s due to timeout"), *GetName()));
	EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UAttackBaseGameplayAbility::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (CooldownGameplayEffectClass)
		CommitAbilityCooldown(Handle, ActorInfo, ActivationInfo, false);

	// If this is a charge attack, play the attack animation now
	if (ChargeDuration > 0.0001f)
	{
		// reset charged status for next invocation
		_bIsCharged = false;
		CurrentChargeDuration = 0ms;
		CurrentChargeDamageMultiplier = 1.f;
		if (ShouldCastOnPartialCharge)
		{
			auto owner = Cast<ACharacter>(ActorInfo->OwnerActor);
			auto animInstance = owner->GetMesh()->GetAnimInstance();
			if (animInstance && FireAnimation)
				animInstance->Montage_Play(FireAnimation);
		}
	}

	ResetTimerAndClearEffects(ActorInfo);
	TryPlayComboPause(ActorInfo);
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UAttackBaseGameplayAbility::TryPlayComboPause(const FGameplayAbilityActorInfo* ActorInfo)
{
	auto owner = Cast<ACharacter>(ActorInfo->OwnerActor);
	if (!owner)
	{
		return;
	}
	auto animInstance = owner->GetMesh()->GetAnimInstance();
	// only play the pause animation if the combo pause animation is set and nothing is playing in the slot
	if (animInstance && ComboPauseAnimation && !animInstance->IsSlotActive(ComboPauseAnimation->SlotAnimTracks[0].SlotName))
	{
		animInstance->Montage_Play(ComboPauseAnimation);
	}
}

void UAttackBaseGameplayAbility::OnMontageCancelled()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UAttackBaseGameplayAbility::OnEffectRemoved(const FGameplayEffectRemovalInfo& GameplayEffectRemovalInfo)
{
	ActiveEffectHandles.Remove(GameplayEffectRemovalInfo.ActiveEffect->Handle);
	if (ActiveEffectHandles.IsEmpty())
	{
		if (GEngine)
			GEngine->AddOnScreenDebugMessage(-1, 1.5f, FColor::Yellow, FString::Printf(TEXT("Ending ability %s due to effect removal"), *GetName()));
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	}
}

void UAttackBaseGameplayAbility::OnMontageCompleted()
{
	// check if effects are still active, if so keep the ability alive
	if (ActiveEffectHandles.Num() > 0)
		return;

	if (GEngine)
		GEngine->AddOnScreenDebugMessage(-1, 1.5f, FColor::Yellow, FString::Printf(TEXT("Ending ability %s due to montage finish"), *GetName()));
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

bool UAttackBaseGameplayAbility::WillCancelAbility(FGameplayAbilitySpec* OtherAbility)
{
	if (CancelAbilitiesWithTag.Num() == 0)
		return false;

	return OtherAbility->Ability->AbilityTags.HasAny(CancelAbilitiesWithTag);
}

bool UAttackBaseGameplayAbility::IsCharged()
{
	return _bIsCharged;
}

void UAttackBaseGameplayAbility::HandleChargeProgress()
{
	if (IsCharged())
	{
		// if we're charged, do nothing
		return;
	}
	// otherwise update the charge progress
	CurrentChargeDuration = duration_cast<milliseconds>(system_clock::now().time_since_epoch()) - ActivationTime;
	
	// calculate the current charge damage multiplier, it will never be below 1
	CurrentChargeDamageMultiplier = 1.f + ((CurrentChargeDuration / (ChargeDuration * 1000ms)) * (FullChargeDamageMultiplier - 1.f));
	if (CurrentChargeDuration >= (ChargeDuration * 1000ms))
	{
		CurrentChargeDamageMultiplier = FullChargeDamageMultiplier;
		_bIsCharged = true;
		if (GEngine)
			GEngine->AddOnScreenDebugMessage(-1, 1.5f, FColor::Yellow, FString::Printf(TEXT("Ability %s is now charged with multiplier of %f"), *GetName(), CurrentChargeDamageMultiplier));
	}
}

TArray<TObjectPtr<UAttackBaseGameplayAbility>> UAttackBaseGameplayAbility::GetAbilityActiveInstances(FGameplayAbilitySpec* spec)
{
	TArray<TObjectPtr<UAttackBaseGameplayAbility>> returnVal;
	auto instances = GetReplicationPolicy() == EGameplayAbilityReplicationPolicy::ReplicateNo
		? spec->NonReplicatedInstances : spec->ReplicatedInstances;

	for (auto instance : instances)
	{
		returnVal.Add(Cast<UAttackBaseGameplayAbility>(instance));
	}

	return returnVal;
}
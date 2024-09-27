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
	ABattlemageTheEndlessCharacter* character = Cast<ABattlemageTheEndlessCharacter>(ActorInfo->OwnerActor);
	if (!world || !character)
	{
		return;
	}

	if (ProjectileClass)
	{
		SpawnProjectile(ActorInfo, character, world);
	}

	// Try and play the sound if specified
	// TODO: Migrate to GameplayCue
	if (FireSound)
		UGameplayStatics::PlaySoundAtLocation(this, FireSound, character->GetActorLocation());

	// Try and play a firing animation if specified
	auto animInstance = character->GetMesh()->GetAnimInstance();
	float montageDuration = 0.f;
	if (FireAnimation && animInstance)
	{
		// TODO: Add a montage rate parameter to the ability
		auto task = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, FireAnimation, 
			1.0f, NAME_None, true, 1.0f, 0.f, true);
		// this is firing way earlier than expected, so we'll use the OnCompleted event instead
		//task->OnBlendOut.AddDynamic(this, &UAttackBaseGameplayAbility::OnMontageCompleted);
		task->OnCompleted.AddDynamic(this, &UAttackBaseGameplayAbility::OnMontageCompleted);
		task->OnInterrupted.AddDynamic(this, &UAttackBaseGameplayAbility::OnMontageCancelled);
		task->OnCancelled.AddDynamic(this, &UAttackBaseGameplayAbility::OnMontageCancelled);
		task->ReadyForActivation();

		// TODO: account for rate parameter when added
		montageDuration = FireAnimation->GetPlayLength();
	}

	if (CooldownGameplayEffectClass)
		CommitAbilityCooldown(Handle, ActorInfo, ActivationInfo, false);

	CommitAbility(Handle, ActorInfo, ActivationInfo);

	bool durationEffectsApplied = false;

	// Apply effects to the character, these will in turn spawn any configured cues (Particles and/or sound)
	for (TSubclassOf<UGameplayEffect> effect : EffectsToApply)
	{
		FGameplayEffectContextHandle context = character->AbilitySystemComponent->MakeEffectContext();
		context.AddSourceObject(character);
		context.AddInstigator(character, ActorInfo->AvatarActor.Get());
		context.AddActors({ character });

		FGameplayEffectSpecHandle specHandle = character->AbilitySystemComponent->MakeOutgoingSpec(effect, 1.f, context);
		if (specHandle.IsValid())
		{
			auto handle = character->AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*specHandle.Data.Get());
			ActiveEffectHandles.Add(handle);

			// TODO: Figure out what to do with infinite effects (keep alive?)
			if (effect.GetDefaultObject()->DurationPolicy != EGameplayEffectDurationType::HasDuration)
				continue;
			
			auto task = UAbilityTask_WaitGameplayEffectRemoved::WaitForGameplayEffectRemoved(this, handle);
			task->OnRemoved.AddDynamic(this, &UAttackBaseGameplayAbility::OnEffectRemoved);
			task->ReadyForActivation();
			durationEffectsApplied = true;
		}
	}

	// If any effects were applied, don't set an end timer, let the effect tasks handle the end
	if (durationEffectsApplied)
		return;

	// If we have no montage and no effects, end the ability immediately
	if (montageDuration < 0.001f)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	// if the effects have the longer duration, set a timer to end the ability after that duration
	FTimerDelegate TimerDelegate = FTimerDelegate::CreateUObject(this, &UAttackBaseGameplayAbility::EndAbility, Handle, ActorInfo, ActivationInfo, true, true);
	world->GetTimerManager().SetTimer(EndTimerHandle, TimerDelegate, montageDuration, false);
}

void UAttackBaseGameplayAbility::SpawnProjectile(const FGameplayAbilityActorInfo* ActorInfo, ABattlemageTheEndlessCharacter* character, UWorld* const world)
{
	const FRotator SpawnRotation = ActorInfo->PlayerController->PlayerCameraManager->GetCameraRotation();
	// MuzzleOffset is in camera space, so transform it to world space before offsetting from the character location to find the final muzzle position
	// Spells are always in the off hand for now, so get the offhand socket
	FName socketName = character->LeftHanded ? FName("GripRight") : FName("GripLeft");

	// Spawn the projectile at the attachment point of the weapon with respect for offsets
	const FVector SpawnLocation = character->GetMesh()->GetSocketLocation(socketName) + SpawnRotation.RotateVector(character->CurrentGripOffset(socketName));

	//Set Spawn Collision Handling Override
	FActorSpawnParameters ActorSpawnParams;
	ActorSpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	// Spawn the projectile at the muzzle
	auto newActor = world->SpawnActor<ABattlemageTheEndlessProjectile>(ProjectileClass, SpawnLocation, SpawnRotation, ActorSpawnParams);
	newActor->GetCollisionComp()->IgnoreActorWhenMoving(character, true);

	// get all actors attached to the character and ignore them
	TArray<AActor*> attachedActors;
	character->GetAttachedActors(attachedActors);
	for (AActor* actor : attachedActors)
	{
		newActor->GetCollisionComp()->IgnoreActorWhenMoving(actor, true);
	}
}

// deprecated in favor of combomanager
// TODO: Remove once we are sure we don't need this
void UAttackBaseGameplayAbility::UpdateComboState(ABattlemageTheEndlessCharacter* character)
{
	// if this ability has the State.Combo tag set or update ExplicitTags to include the State.Combo tag
	FGameplayTagContainer ComboTags;
	ComboTags.AddTag(FGameplayTag::RequestGameplayTag(FName("State.Combo")));

	FGameplayTagContainer ownedComboTags = AbilityTags.Filter(ComboTags);
	if (ownedComboTags.Num() > 1)
	{
		UE_LOG(LogTemp, Error, TEXT("Ability has more than one State.Combo.# tag"));
	}
	else if (ownedComboTags.Num() == 1)
	{
		FGameplayTag StateComboTag = ownedComboTags.GetByIndex(0);
		character->AbilitySystemComponent->UpdateTagMap(StateComboTag, 1);
	}
}

void UAttackBaseGameplayAbility::CancelAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateCancelAbility)
{
	ResetTimerAndClearEffects(ActorInfo);
	Super::CancelAbility(Handle, ActorInfo, ActivationInfo, bReplicateCancelAbility);
}

void UAttackBaseGameplayAbility::ResetTimerAndClearEffects(const FGameplayAbilityActorInfo* ActorInfo)
{
	// if we have a timer running to end the ability, clear it
	UWorld* const world = GetWorld();
	if (world && world->GetTimerManager().IsTimerActive(EndTimerHandle))
	{
		world->GetTimerManager().ClearTimer(EndTimerHandle);
	}

	// TODO: Handle this unsafe cast (OwnerActor can be null)
	//ABattlemageTheEndlessCharacter* character = Cast<ABattlemageTheEndlessCharacter>(ActorInfo->OwnerActor);
	//for (TSubclassOf<UGameplayEffect> effect : EffectsToApply)
	//{
	//	character->AbilitySystemComponent->RemoveActiveGameplayEffectBySourceEffect(effect, character->AbilitySystemComponent, 1);
	//}
}

void UAttackBaseGameplayAbility::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	ResetTimerAndClearEffects(ActorInfo);
	TryPlayComboPause(ActorInfo);
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UAttackBaseGameplayAbility::TryPlayComboPause(const FGameplayAbilityActorInfo* ActorInfo)
{
	ABattlemageTheEndlessCharacter* character = Cast<ABattlemageTheEndlessCharacter>(ActorInfo->OwnerActor);
	if (!character)
	{
		return;
	}
	auto animInstance = character->GetMesh()->GetAnimInstance();
	// only play the pause animation if the combo pause animation is set and nothing is playing in the slot
	if (animInstance && ComboPauseAnimation && !animInstance->IsSlotActive(ComboPauseAnimation->SlotAnimTracks[0].SlotName))
	{
		animInstance->Montage_Play(ComboPauseAnimation);
	}
}

void UAttackBaseGameplayAbility::OnMontageCancelled()
{
	EndAbility(this->CurrentSpecHandle, this->CurrentActorInfo, this->CurrentActivationInfo, true, true);
}

void UAttackBaseGameplayAbility::OnEffectRemoved(const FGameplayEffectRemovalInfo& GameplayEffectRemovalInfo)
{
	ActiveEffectHandles.Remove(GameplayEffectRemovalInfo.ActiveEffect->Handle);
	if (ActiveEffectHandles.IsEmpty())
	{
		if (GEngine)
			GEngine->AddOnScreenDebugMessage(-1, 1.5f, FColor::Yellow, FString::Printf(TEXT("Ending ability %s due to effect removal"), *GetName()));
		EndAbility(this->CurrentSpecHandle, this->CurrentActorInfo, this->CurrentActivationInfo, true, false);
	}
}

void UAttackBaseGameplayAbility::OnMontageCompleted()
{
	// check if effects are still active, if so keep the ability alive
	if (ActiveEffectHandles.Num() > 0)
		return;

	if (GEngine)
		GEngine->AddOnScreenDebugMessage(-1, 1.5f, FColor::Yellow, FString::Printf(TEXT("Ending ability %s due to montage finish"), *GetName()));
	EndAbility(this->CurrentSpecHandle, this->CurrentActorInfo, this->CurrentActivationInfo, true, false);
}

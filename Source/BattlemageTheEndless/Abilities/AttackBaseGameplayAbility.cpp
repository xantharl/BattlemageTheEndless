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
	// populate tags to look for
	FGameplayTagContainer comboStateTags = FGameplayTagContainer(FGameplayTag::RequestGameplayTag(FName("State.Combo")));
	auto comboTags = AbilityTags.Filter(comboStateTags);

	if (comboTags.Num() > 1)
	{
		UE_LOG(LogTemp, Error, TEXT("Ability %s has more than one State.Combo tag. This is not supported."), *GetName());
		return false;
	}

	return comboTags.Num() > 0;
}

bool UAttackBaseGameplayAbility::IsFirstInCombo()
{
	// populate tags to look for
	FGameplayTagContainer comboStateTags = FGameplayTagContainer(FGameplayTag::RequestGameplayTag(FName("State.Combo")));
	auto comboTags = AbilityTags.Filter(comboStateTags);

	if (comboTags.Num() > 1)
	{
		UE_LOG(LogTemp, Error, TEXT("Ability %s has more than one State.Combo tag. This is not supported."), *GetName());
		return false;
	}

	return comboTags.Num() > 0 && comboTags.First().ToString().EndsWith(".1");
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
		}
	}

	// Try and play the sound if specified
	if (FireSound)
		UGameplayStatics::PlaySoundAtLocation(this, FireSound, character->GetActorLocation());

	// Try and play a firing animation if specified
	// TODO: Make this use AbilitySystemComponent->PlayMontage
	auto animInstance = character->GetMesh()->GetAnimInstance();
	if (FireAnimation && animInstance)
	{
		// in the old system we resumed an ongoing montage, but now we are requiring a montage per phase of the ability
		animInstance->Montage_Play(FireAnimation, 1.f);
	}

	if (CooldownGameplayEffectClass)
		CommitAbilityCooldown(Handle, ActorInfo, ActivationInfo, false);

	CommitAbility(Handle, ActorInfo, ActivationInfo);
	//UpdateComboState(character);
	// find longest duration gameplay effect if any exist
	float longestDuration = 0.f;
	for (TSubclassOf<UGameplayEffect> effect : EffectsToApply)
	{
		UGameplayEffect* effectCDO = effect.GetDefaultObject();
		if (effectCDO->DurationPolicy == EGameplayEffectDurationType::HasDuration)
		{
			float duration;
			effectCDO->DurationMagnitude.GetStaticMagnitudeIfPossible(0, duration, nullptr);
			longestDuration = FMath::Max(duration, longestDuration);
		}
	}

	// keep the ability alive if any effects have durations
	// TODO: Account for infinite effects
	if (longestDuration > 0.001f)
	{
		// set a timer to end the ability after the longest duration of any effect
		FTimerDelegate TimerDelegate = FTimerDelegate::CreateUObject(this, &UAttackBaseGameplayAbility::EndAbility, Handle, ActorInfo, ActivationInfo, true, true);
		world->GetTimerManager().SetTimer(EndTimerHandle, TimerDelegate, longestDuration, false);
	}
	else
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
	}
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
	// if we have a timer running to end the ability, clear it
	UWorld* const world = GetWorld();
	if (world)
	{
		world->GetTimerManager().ClearTimer(EndTimerHandle);
	}

	// TODO: Handle this unsafe cast (OwnerActor can be null)
	ABattlemageTheEndlessCharacter* character = Cast<ABattlemageTheEndlessCharacter>(ActorInfo->OwnerActor);
	for (TSubclassOf<UGameplayEffect> effect : EffectsToApply)
	{
		character->AbilitySystemComponent->RemoveActiveGameplayEffectBySourceEffect(effect, character->AbilitySystemComponent, 1);
	}

	Super::CancelAbility(Handle, ActorInfo, ActivationInfo, bReplicateCancelAbility);
}

// Fill out your copyright notice in the Description page of Project Settings.

#include "AttackBaseGameplayAbility.h"

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

	if (AttackEffect.NiagaraSystem)
	{
		SpawnAttackEffect(ActorInfo, character, world);
	}

	// Try and play the sound if specified
	if (FireSound)
		UGameplayStatics::PlaySoundAtLocation(this, FireSound, character->GetActorLocation());

	// Try and play a firing animation if specified
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
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

void UAttackBaseGameplayAbility::SpawnAttackEffect(const FGameplayAbilityActorInfo* ActorInfo, ABattlemageTheEndlessCharacter* character, UWorld* const world)
{
	const FRotator SpawnRotation = ActorInfo->PlayerController->PlayerCameraManager->GetCameraRotation();
	FName socketName = FName("cameraSocket");

	// Spawn the system at the attachment point of the camera plus the rotated offset
	FVector SpawnLocation = character->GetMesh()->GetSocketLocation(socketName)
		+ AttackEffect.SpawnOffset.RotateAngleAxis(SpawnRotation.Yaw, FVector::ZAxisVector);

	// handle bSnapToGround
	if (AttackEffect.bSnapToGround)
	{
		FHitResult hitResult;
		const FVector end = SpawnLocation - FVector(0.f, 0.f, 1000.f);
		world->LineTraceSingleByChannel(hitResult, SpawnLocation, end, ECollisionChannel::ECC_Visibility);
		if (hitResult.bBlockingHit)
		{
			SpawnLocation = hitResult.ImpactPoint;
		}
	}

	//Set Spawn Collision Handling Override
	FActorSpawnParameters ActorSpawnParams;
	ActorSpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	if (AttackEffect.AttachSocket.IsNone())
	{
		AttackEffect.NiagaraComponentInstance = UNiagaraFunctionLibrary::SpawnSystemAtLocation(world, AttackEffect.NiagaraSystem,
			SpawnLocation, SpawnRotation, FVector(1.f), false, true, ENCPoolMethod::None, false);
	}
	else
	{
		AttackEffect.NiagaraComponentInstance = UNiagaraFunctionLibrary::SpawnSystemAttached(AttackEffect.NiagaraSystem, character->GetMesh(),
			AttackEffect.AttachSocket, FVector::ZeroVector, FRotator::ZeroRotator, EAttachLocation::KeepRelativeOffset, false,
			true, ENCPoolMethod::None, false);
	}

	// todo: figure out collision handling

	// kill from AttackEffect.bShouldKillPreviousEffect is handled by UAbilityComboManager::ActivateAbilityAndResetTimer
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

// Fill out your copyright notice in the Description page of Project Settings.


#include "BMageGameplayAbility.h"

void UBMageGameplayAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	UWorld* const world = GetWorld();
	ABattlemageTheEndlessCharacter* character = Cast<ABattlemageTheEndlessCharacter>(ActorInfo->OwnerActor);
	if (!world || !character)
	{
		return;
	}

	if (ProjectileClass)
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

	// Try and play the sound if specified
	if (FireSound)
		UGameplayStatics::PlaySoundAtLocation(this, FireSound, character->GetActorLocation());

	// Try and play a firing animation if specified
	if (FireAnimation && ActorInfo->AnimInstance != nullptr)
	{
		// in the old system we resumed an ongoing montage, but now we are requiring a montage per phase of the ability
		ActorInfo->AnimInstance->Montage_Play(FireAnimation, 1.f);
	}
}

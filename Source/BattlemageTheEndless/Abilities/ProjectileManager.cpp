// Fill out your copyright notice in the Description page of Project Settings.


#include "ProjectileManager.h"

TArray<ABattlemageTheEndlessProjectile*> UProjectileManager::SpawnProjectiles_Actor(
	UAttackBaseGameplayAbility* spawningAbility, const FProjectileConfiguration& configuration, AActor* actor)
{
	auto returnArray = TArray<ABattlemageTheEndlessProjectile*>();
	auto spawnLocations = GetSpawnLocations(configuration, actor->GetTransform());
	TArray<ABattlemageTheEndlessProjectile*> projectiles = HandleSpawn(spawnLocations, configuration, spawningAbility, actor);
	return projectiles;
}

TArray<ABattlemageTheEndlessProjectile*> UProjectileManager::SpawnProjectiles_Location(
	UAttackBaseGameplayAbility* spawningAbility, const FProjectileConfiguration& configuration, const FRotator rotation,
	const FVector translation, const FVector scale, AActor* ignoreActor)
{
	auto returnArray = TArray<ABattlemageTheEndlessProjectile*>();
	auto spawnLocations = GetSpawnLocations(configuration, FTransform(rotation, translation, scale));
	TArray<ABattlemageTheEndlessProjectile*> projectiles = HandleSpawn(spawnLocations, configuration, spawningAbility, ignoreActor);
	return projectiles;
}

TArray<ABattlemageTheEndlessProjectile*> UProjectileManager::HandleSpawn(FTransformArrayA2& spawnLocations, const FProjectileConfiguration& configuration, 
	UAttackBaseGameplayAbility* spawningAbility, AActor* ignoreActor)
{
	TArray<ABattlemageTheEndlessProjectile*> returnArray = TArray<ABattlemageTheEndlessProjectile*>();
	auto world = GetWorld();
	FActorSpawnParameters ActorSpawnParams = FActorSpawnParameters();
	ActorSpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	if (!world)
	{
		UE_LOG(LogTemp, Error, TEXT("No world found for projectile spawning"));
		return returnArray;
	}

	TArray<AActor*> attachedActors;
	if (OwnerCharacter)
	{
		attachedActors.Add(OwnerCharacter);
		OwnerCharacter->GetAttachedActors(attachedActors);
	}

	for (const FTransform& location : spawnLocations)
	{
		const FVector SpawnLocation = location.GetLocation();
		const FRotator SpawnRotation = location.GetRotation().Rotator();

		auto newActor = world->SpawnActor<ABattlemageTheEndlessProjectile>(
			configuration.ProjectileClass, SpawnLocation, SpawnRotation, ActorSpawnParams);

		newActor->SpawningAbility = spawningAbility;
		newActor->OwnerActor = OwnerCharacter;

		// get all actors attached to the OwnerCharacter and ignore them
		for (AActor* actor : attachedActors)
			newActor->GetCollisionComp()->IgnoreActorWhenMoving(actor, true);

		returnArray.Add(newActor);
	}

	return returnArray;
}

TArray<FTransform> UProjectileManager::GetSpawnLocations(const FProjectileConfiguration& configuration, const FTransform& rootTransform)
{
	// Some day we'll make this smarter but for now it's basically a bunch of if statements checking on the shape and location

	// For one projectile, we really only care about the offset
	if(configuration.Amount == 1)
	{		
		auto spawnTranslation = rootTransform.GetTranslation() + configuration.SpawnOffset.RotateAngleAxis(rootTransform.Rotator().Yaw, FVector::ZAxisVector);
		auto spawnTransform = FTransform(rootTransform.GetRotation(), spawnTranslation, 	rootTransform.GetScale3D());
		return TArray<FTransform>{spawnTransform};
	}

	// TODO: Implement shapes

	return TArray<FTransform>();
}

void UProjectileManager::OnProjectileDestroyed(AActor* destroyedActor)
{
	//destroyedActor->Destroy();
}

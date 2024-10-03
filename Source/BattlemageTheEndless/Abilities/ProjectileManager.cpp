// Fill out your copyright notice in the Description page of Project Settings.


#include "ProjectileManager.h"

TArray<ABattlemageTheEndlessProjectile*> UProjectileManager::SpawnProjectiles_Actor(
	TSubclassOf<class UGameplayAbility> spawningAbilityClass, const FProjectileConfiguration& configuration, const AActor* actor)
{
	auto returnArray = TArray<ABattlemageTheEndlessProjectile*>();
	auto spawnLocations = GetSpawnLocations(configuration, actor->GetTransform());
	return HandleSpawn(spawnLocations, configuration);
}

TArray<ABattlemageTheEndlessProjectile*> UProjectileManager::SpawnProjectiles_Transform(
	TSubclassOf<class UGameplayAbility> spawningAbilityClass, const FProjectileConfiguration& configuration, const FTransform& transform)
{
	auto returnArray = TArray<ABattlemageTheEndlessProjectile*>();
	auto spawnLocations = GetSpawnLocations(configuration, transform);
	return HandleSpawn(spawnLocations, configuration);
}

TArray<ABattlemageTheEndlessProjectile*> UProjectileManager::HandleSpawn(FTransformArrayA2& spawnLocations, const FProjectileConfiguration& configuration)
{
	TArray<ABattlemageTheEndlessProjectile*> returnArray = TArray<ABattlemageTheEndlessProjectile*>();
	auto world = GetWorld();
	if (!world)
	{
		UE_LOG(LogTemp, Error, TEXT("No world found for projectile spawning"));
		return returnArray;
	}
	for (auto location : spawnLocations)
	{
		const FVector SpawnLocation = location.GetLocation();
		const FRotator SpawnRotation = location.GetRotation().Rotator();
		FActorSpawnParameters ActorSpawnParams = FActorSpawnParameters();
		ActorSpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		returnArray.Add(world->SpawnActor<ABattlemageTheEndlessProjectile>(
			configuration.ProjectileClass, SpawnLocation, SpawnRotation, ActorSpawnParams));
	}

	return returnArray;
}

TArray<FTransform> UProjectileManager::GetSpawnLocations(const FProjectileConfiguration& configuration, const FTransform& rootTransform)
{
	// Some day we'll make this smarter but for now it's basically a bunch of if statements checking on the shape and location

	// For one projectile, we really only care about the offset
	if(configuration.Amount == 1)
	{
		rootTransform.GetTranslation() + configuration.SpawnOffset.RotateAngleAxis(rootTransform.GetRotation().Z, FVector::ZAxisVector);
		return TArray<FTransform>{rootTransform};
	}

	// TODO: Implement shapes

	return TArray<FTransform>();
}

void UProjectileManager::StoreProjectileInstance(FAbilityInstanceProjectiles instance)
{
	for (auto projectile : instance.Projectiles)
	{
		projectile->OnDestroyed.AddDynamic(this, &UProjectileManager::OnProjectileDestroyed);
	}
	ActiveProjectiles.Add(instance);
}

void UProjectileManager::OnProjectileDestroyed(AActor* destroyedActor)
{
	// TODO: This isn't a terribly efficient way to do this, refine later
	for (int i = 0; i < ActiveProjectiles.Num(); i++)
	{
		// If we're not dealing with the same projectile class, skip
		if (ActiveProjectiles[i].Configuration.ProjectileClass != destroyedActor->StaticClass())
			continue;

		// Call remove, if it finds any to remove, we're done
		auto projectile = Cast<ABattlemageTheEndlessProjectile>(destroyedActor);
		if (ActiveProjectiles[i].Projectiles.Remove(projectile) > 0)
		{
			// if this instance has no more projectiles, remove it from the active list
			if (ActiveProjectiles[i].Projectiles.Num() == 0)
				ActiveProjectiles.RemoveAt(i);

			break;
		}
	}
}

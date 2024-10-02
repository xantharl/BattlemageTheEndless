// Fill out your copyright notice in the Description page of Project Settings.


#include "ProjectileManager.h"

TArray<ABattlemageTheEndlessProjectile*> UProjectileManager::SpawnProjectiles_Actor(FProjectileConfiguration& configuration, AActor* actor)
{
	auto returnArray = TArray<ABattlemageTheEndlessProjectile*>();
	// get the spawn locations and rotations
	auto spawnLocations = GetSpawnLocations(configuration);
	/*auto newActor = GetWorld()->SpawnActor<ABattlemageTheEndlessProjectile>(
		configuration.ProjectileClass, SpawnLocation, SpawnRotation, ActorSpawnParams);*/

	return returnArray;
}

TArray<FTransform> UProjectileManager::GetSpawnLocations(FProjectileConfiguration& configuration)
{
	return TArray<FTransform>();
}

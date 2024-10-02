// Fill out your copyright notice in the Description page of Project Settings.


#include "ProjectileManager.h"

TArray<ABattlemageTheEndlessProjectile*> UProjectileManager::SpawnProjectiles_Actor(const FProjectileConfiguration& configuration, const AActor* actor)
{
	auto returnArray = TArray<ABattlemageTheEndlessProjectile*>();
	auto spawnLocations = GetSpawnLocations(configuration, actor->GetTransform());
	/*auto newActor = GetWorld()->SpawnActor<ABattlemageTheEndlessProjectile>(
		configuration.ProjectileClass, SpawnLocation, SpawnRotation, ActorSpawnParams);*/

	return returnArray;
}

TArray<ABattlemageTheEndlessProjectile*> UProjectileManager::SpawnProjectiles_Transform(const FProjectileConfiguration& configuration, const FTransform& transform)
{
	auto returnArray = TArray<ABattlemageTheEndlessProjectile*>();
	auto spawnLocations = GetSpawnLocations(configuration, transform);
	return returnArray;
}

TArray<FTransform> UProjectileManager::GetSpawnLocations(const FProjectileConfiguration& configuration, const FTransform& rootTransform)
{
	return TArray<FTransform>();
}
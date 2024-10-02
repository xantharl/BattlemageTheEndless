// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Abilities/GameplayAbility.h"
#include "../Pickups/BattlemageTheEndlessProjectile.h"
#include "../Abilities/PersistentAreaEffect.h"
#include "ProjectileManager.generated.h"

USTRUCT(BlueprintType)
struct FAbilityInstanceProjectiles
{
	GENERATED_BODY()

public:
	/** Ability class which spawned these projectiles */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Projectile, meta = (AllowPrivateAccess = "true"))
	TSubclassOf<class UGameplayAbility> SpawningAbilityClass;

	/** Configuration definining how to spawn projectiles */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Projectile, meta = (AllowPrivateAccess = "true"))
	FProjectileConfiguration Configuration;

	/** Array of spawned projectiles */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Projectile, meta = (AllowPrivateAccess = "true"))
	TArray<ABattlemageTheEndlessProjectile*> Projectiles;

	// TODO: Add delegate for projectile destruction so that the manager can remove the instance from the active list
};

/**
 * Projectile Manager for handling projectile spawning and management
 *	from gameplay abilities.
 */
UCLASS()
class BATTLEMAGETHEENDLESS_API UProjectileManager : public UObject
{
	GENERATED_BODY()

	// Spawns projectiles based on the provided configuration and actor location
	TArray<ABattlemageTheEndlessProjectile*> SpawnProjectiles_Actor(const FProjectileConfiguration& configuration, const AActor* actor);

	// Spawns projectiles based on the provided configuration and specific transform, used for "previous ability" spawn location
	TArray<ABattlemageTheEndlessProjectile*> SpawnProjectiles_Transform(const FProjectileConfiguration& configuration, const FTransform& transform);

private:
	// Produces spawn locations and rotations (relative) based on the provided configuration
	TArray<FTransform> GetSpawnLocations(const FProjectileConfiguration& configuration, const FTransform& rootTransform);

	TArray<FAbilityInstanceProjectiles> ActiveProjectiles;

	// TODO: Subscribe to projectile destruction events to remove instances from the active list
};

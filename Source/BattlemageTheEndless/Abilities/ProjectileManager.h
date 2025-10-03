// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Abilities/GameplayAbility.h"
#include "../Pickups/BattlemageTheEndlessProjectile.h"
#include "../Abilities/PersistentAreaEffect.h"
#include "../Abilities/AttackBaseGameplayAbility.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "ProjectileManager.generated.h"

USTRUCT(BlueprintType)
struct FAbilityInstanceProjectiles
{
	GENERATED_BODY()

public:
	/** Ability class which spawned these projectiles */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Projectile, meta = (AllowPrivateAccess = "true"))
	UAttackBaseGameplayAbility* SpawningAbility;

	/** Configuration definining how to spawn projectiles */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Projectile, meta = (AllowPrivateAccess = "true"))
	FProjectileConfiguration Configuration;

	// TODO: Can we just use AActor?
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

public:
	void Initialize(ACharacter* character) { OwnerCharacter = character; }

	// Spawns projectiles entry point based on the provided configuration and actor location
	TArray<ABattlemageTheEndlessProjectile*> SpawnProjectiles_Actor(UAttackBaseGameplayAbility* spawningAbility, const FProjectileConfiguration& configuration, AActor* actor);

	// Spawns projectiles entry point based on the provided configuration and specific transform, used for "previous ability" spawn location
	TArray<ABattlemageTheEndlessProjectile*> SpawnProjectiles_Location(UAttackBaseGameplayAbility* spawningAbility, const FProjectileConfiguration& configuration,
		const FRotator rotation, const FVector translation, const FVector scale = FVector::OneVector, AActor* ignoreActor = nullptr);

	// TODO: Make these all members of the theoretical shapes classes
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ProjectileConstants, meta = (AllowPrivateAccess = "true"))
	float ConeStartSize = 50.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ProjectileConstants, meta = (AllowPrivateAccess = "true"))
	int ConeOuterPoints = 8;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ProjectileConstants, meta = (AllowPrivateAccess = "true"))
	float LineSpacing = 50.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ProjectileConstants, meta = (AllowPrivateAccess = "true"))
	float RingDiameter = 50.f;

	ACharacter* OwnerCharacter;
private:

	// Actual spawner
	TArray<ABattlemageTheEndlessProjectile*> HandleSpawn(FTransformArrayA2& spawnLocations, const FProjectileConfiguration& configuration, 
		UAttackBaseGameplayAbility* spawningAbility, AActor* ignoreActor = nullptr);

	// Produces spawn locations and rotations (relative) based on the provided configuration
	TArray<FTransform> GetSpawnLocations(const FProjectileConfiguration& configuration, const FTransform& rootTransform);

	void GetRingPoints(const FProjectileConfiguration& configuration, const FTransform& spawnTransform, FTransformArrayA2& returnArray, bool bInwards);

	void GetLinePoints(const FProjectileConfiguration& configuration, FTransformArrayA2& returnArray, FTransform& spawnTransform);

	// Delegate for projectile destruction
	UFUNCTION()
	void OnProjectileDestroyed(AActor* destroyedActor);

	// TODO: Subscribe to projectile destruction events to remove instances from the active list
};

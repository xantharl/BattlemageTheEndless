// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "../Pickups/BattlemageTheEndlessProjectile.h"
#include "../Abilities/AttackBaseGameplayAbility.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "ProjectileManager.generated.h"

/**
 * Projectile Manager for handling projectile spawning and management
 *	from gameplay abilities.
 */
UCLASS()
class BATTLEMAGETHEENDLESS_API UProjectileManager : public UObject
{
	GENERATED_BODY()

public:
	void Initialize(ACharacter* character);

	// Spawns projectiles entry point based on the provided configuration and actor location
	void SpawnProjectiles_Actor(UAttackBaseGameplayAbility* spawningAbility, const FProjectileConfiguration& configuration, AActor* actor);

	// Spawns projectiles entry point based on the provided configuration and specific transform, used for "previous ability" and "spell focus" spawn location
	void SpawnProjectiles_Location(UAttackBaseGameplayAbility* spawningAbility, const FProjectileConfiguration& configuration,
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

	UPROPERTY(Instanced)
	ACharacter* OwnerCharacter;
private:
	UFUNCTION(Server, Reliable)
	void HandleSpawn(const TArray<FTransform>& spawnLocations, const FProjectileConfiguration& configuration, 
		const UAttackBaseGameplayAbility* spawningAbility, const AActor* ignoreActor = nullptr);

	// Produces spawn locations and rotations (relative) based on the provided configuration
	TArray<FTransform> GetSpawnLocations(const FProjectileConfiguration& configuration, const FTransform& rootTransform, UAttackBaseGameplayAbility* spawningAbility);

	void GetRingPoints(const FProjectileConfiguration& configuration, const FTransform& spawnTransform, FTransformArrayA2& returnArray, bool bInwards);

	void GetLinePoints(const FProjectileConfiguration& configuration, FTransformArrayA2& returnArray, FTransform& spawnTransform);

	// Delegate for projectile destruction
	UFUNCTION()
	void OnProjectileDestroyed(AActor* destroyedActor);

	FTransform GetOwnerLookAtTransform();

	TArray<const UCameraComponent*> _ownerCameras;

	// TODO: Subscribe to projectile destruction events to remove instances from the active list
};

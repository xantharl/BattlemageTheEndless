// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Abilities/GameplayAbility.h"
#include "../Pickups/BattlemageTheEndlessProjectile.h"
#include "../Abilities/PersistentAreaEffect.h"
#include "ProjectileManager.generated.h"

/// <summary>
/// Enum of possible shapes for spawning projectiles
/// </summary>
UENUM(BlueprintType)
enum class FSpawnShape : uint8
{
	None UMETA(DisplayName = "None"),
	Cone UMETA(DisplayName = "Cone"),
	// Differs from Cone by being a flat shape
	Fan UMETA(DisplayName = "Fan"),
	Line UMETA(DisplayName = "Line"),
	InwardRing UMETA(DisplayName = "InwardRing"),
	OutwardRing UMETA(DisplayName = "OutwardRing"),
	Rain UMETA(DisplayName = "Rain"),
	Erruption UMETA(DisplayName = "Erruption")
};

/// <summary>
/// Enum of possible locations for spawning projectiles
/// </summary>
UENUM(BlueprintType)
enum class FSpawnLocation : uint8
{
	// Spawns at the spell orb's location, targets based on a raycast from the camera
	SpellFocus UMETA(DisplayName = "SpellFocus"),
	// Spawns at the player's location and fires straight (after accounting for any rotation based on the spawn shape)
	Player UMETA(DisplayName = "Player"),
	// Spawns at the location of the last phase of this ability combo
	PreviousAbility UMETA(DisplayName = "PreviousAbility")
};

UENUM(BlueprintType)
enum class FTargetType : uint8
{
	// Only affects targets on a direct hit
	Single UMETA(DisplayName = "Single"),
	// Affects all targets in the area of effect
	Area UMETA(DisplayName = "Area"),
	// Hits the first target and chains to others within range
	Chain UMETA(DisplayName = "Chain")
};

/// <summary>
/// Configuration for spawning projectiles, visibility is controlled by the implementation of the projectile class
/// </summary>
USTRUCT(BlueprintType)
struct FProjectileConfiguration
{
	GENERATED_BODY()

public:

	/** Projectile class to spawn */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Projectile, meta = (AllowPrivateAccess = "true"))
	TSubclassOf<class ABattlemageTheEndlessProjectile> ProjectileClass;

	/** Amount of projectiles to spawn */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Projectile, meta = (AllowPrivateAccess = "true"))
	uint8 Amount = 1;

	/** Time to let the projectile(s) live (seconds) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Projectile, meta = (AllowPrivateAccess = "true"))
	float Lifetime = 1.0;

	/** Distance to let the projectile(s) live, 0 = unlimited */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Projectile, meta = (AllowPrivateAccess = "true"))
	float MaxRange = 0.f;

	/** Shape to spawn Projectiles in, ignored if amount is 1 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Projectile, meta = (AllowPrivateAccess = "true"))
	FSpawnShape Shape = FSpawnShape::None;

	/** Location to spawn Projectiles at */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Projectile, meta = (AllowPrivateAccess = "true"))
	FSpawnLocation Location = FSpawnLocation::SpellFocus;

	/** Projectile speed override to allow reuse of simple projectile instances */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Projectile, meta = (AllowPrivateAccess = "true"))
	float SpeedOverride = 0.f;

	/** Projectile Gravity Scale override to allow reuse of simple projectile instances */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Projectile, meta = (AllowPrivateAccess = "true"))
	float GravityScaleOverride = 0.f;

	/** Total spread of projectiles in degrees, only applicable for Cone and Fan shapes */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Projectile, meta = (AllowPrivateAccess = "true"))
	float Spread = 0.f;

	/** Total radius of projectiles, only applicable for Erruption and Rain shapes */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Projectile, meta = (AllowPrivateAccess = "true"))
	float SpawnRadius = 0.f;

	/** Radius of Hit for area effects */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Projectile, meta = (AllowPrivateAccess = "true"))
	float HitRadius = 0.f;

	/** Persistent Area effect class to activate, only applicable to spells with HitType of Area */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Projectile, meta = (AllowPrivateAccess = "true"))
	TSubclassOf<class APersistentAreaEffect> PersistentAreaEffect = nullptr;
};

USTRUCT(BlueprintType)
struct FAbilityInstanceProjectiles
{
	GENERATED_BODY()

public:
	/** Ability class which spawned these projectiles */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Projectile, meta = (AllowPrivateAccess = "true"))
	TSubclassOf<class UAttackBaseGameplayAbility> SpawningAbilityClass;

	/** Configuration definining how to spawn projectiles */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Projectile, meta = (AllowPrivateAccess = "true"))
	FProjectileConfiguration Configuration;

	/** Array of spawned projectiles */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Projectile, meta = (AllowPrivateAccess = "true"))
	TArray<ABattlemageTheEndlessProjectile*> Projectiles;
};

// TODO: 100+ lines of defining structs for this class before the class is gross
/**
 * Projectile Manager for handling projectile spawning and management
 *	from gameplay abilities.
 */
UCLASS()
class BATTLEMAGETHEENDLESS_API UProjectileManager : public UObject
{
	GENERATED_BODY()

	// Note that some abilities can be re-activated while a projectile from the previous instance 
	//   still lives. For that reason we will not reset the array on ability activation or end
	TArray<ABattlemageTheEndlessProjectile*> SpawnProjectiles_Actor(FProjectileConfiguration& configuration, AActor* actor);

private:
	// Produces spawn locations and rotations (relative) based on the provided configuration
	TArray<FTransform> GetSpawnLocations(FProjectileConfiguration& configuration);
};

// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/SphereComponent.h"
#include "Abilities/GameplayAbility.h"
#include "BattlemageTheEndlessProjectile.generated.h"

class USphereComponent;
class UProjectileMovementComponent;

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
/// Enum of possible shapes for spawning projectiles
/// Note: Some shapes will override projectile count as required to make sense
/// </summary>
UENUM(BlueprintType)
enum class FSpawnShape : uint8
{
	None UMETA(DisplayName = "None"),
	// This shape ignores the Amount property and determines the number of projectiles based on the spread
	Cone UMETA(DisplayName = "Cone"),
	// Differs from Cone by being a flat shape
	Fan UMETA(DisplayName = "Fan"),
	Line UMETA(DisplayName = "Line"),
	InwardRing UMETA(DisplayName = "InwardRing"),
	OutwardRing UMETA(DisplayName = "OutwardRing")
	//Rain UMETA(DisplayName = "Rain")
	//Erruption UMETA(DisplayName = "Erruption")
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

/// <summary>s
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

	/** Time to let the projectile(s) live (seconds), 0 = unlimited */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Projectile, meta = (AllowPrivateAccess = "true"))
	float Lifetime = 0.0;

	/** Shape to spawn Projectiles in, ignored if amount is 1 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Projectile, meta = (AllowPrivateAccess = "true"))
	FSpawnShape Shape = FSpawnShape::None;

	/** Location to spawn Projectiles at. Ignored for HitScan */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Projectile, meta = (AllowPrivateAccess = "true"))
	FSpawnLocation SpawnLocation = FSpawnLocation::SpellFocus;

	/** Projectile speed override to allow reuse of simple projectile instances */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Projectile, meta = (AllowPrivateAccess = "true"))
	float SpeedOverride = 0.f;

	/** Projectile Gravity Scale override to allow reuse of simple projectile instances */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Projectile, meta = (AllowPrivateAccess = "true"))
	float GravityScaleOverride = 0.f;

	/** Total spread of projectiles in degrees, only applicable for Cone and Fan shapes */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Projectile, meta = (AllowPrivateAccess = "true"))
	float Spread = 0.f;

	/** Additional offset to add when spawning the projectile, this is taken into account before any shape decisions */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Projectile, meta = (AllowPrivateAccess = "true"))
	FVector SpawnOffset = FVector::ZeroVector;

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

UCLASS(config=Game)
class ABattlemageTheEndlessProjectile : public AActor
{
	GENERATED_BODY()

public:
	/** Sphere collision component */
	UPROPERTY(VisibleDefaultsOnly, Category=Projectile)
	USphereComponent* CollisionComp;

	/** Projectile movement component */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Movement, meta = (AllowPrivateAccess = "true"))
	UProjectileMovementComponent* ProjectileMovement;

	UGameplayAbility* SpawningAbility;
	AActor* OwnerActor;

	/** Effective Damage for abilities which do not have constant damage, evaluated at time of spawn **/
	UPROPERTY(BlueprintReadOnly, Category = Damage, meta = (AllowPrivateAccess = "true"))
	float EffectiveDamage;

	ABattlemageTheEndlessProjectile();

	/** Called when the projectile hits something, damage is handled by the spawning gameplay ability **/
	UFUNCTION()
	void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	/** Returns CollisionComp subobject **/
	USphereComponent* GetCollisionComp() const { return CollisionComp; }
	/** Returns ProjectileMovement subobject **/
	UProjectileMovementComponent* GetProjectileMovement() const { return ProjectileMovement; }

	virtual bool ShouldDestroyOnHit() { return true;}

	virtual void Tick(float DeltaTime) override;

	/** If set, used on actor tick to scale the projectile over time **/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Growth)
	UCurveFloat* ScaleByTime;
};


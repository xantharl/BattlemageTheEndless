// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BattlemageTheEndless/Pickups/BattlemageTheEndlessProjectile.h"
#include "BattlemageTheEndless/Abilities/AttackBaseGameplayAbility.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/Character.h"
#include "ProjectileManagerComponent.generated.h"

USTRUCT(BlueprintType)
struct FAbilityInstanceProjectiles
{
	GENERATED_BODY()

public:
	/** Ability class which spawned these projectiles */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Projectile, meta = (AllowPrivateAccess = "true"))
	UAttackBaseGameplayAbility* SpawningAbility;

	/** Configuration defining how to spawn projectiles */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Projectile, meta = (AllowPrivateAccess = "true"))
	FProjectileConfiguration Configuration;

	/** Array of spawned projectiles */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Projectile, meta = (AllowPrivateAccess = "true"))
	TArray<ABattlemageTheEndlessProjectile*> Projectiles;

	// TODO: Add delegate for projectile destruction so that the manager can remove the instance from the active list
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class BATTLEMAGETHEENDLESS_API UProjectileManagerComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UProjectileManagerComponent();
	
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
protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	UFUNCTION(Server, Reliable)
	void HandleSpawn(const TArray<FTransform>& spawnLocations, const FProjectileConfiguration& configuration, 
		const FGameplayAbilitySpecHandle& spawningAbilityHandle, const AActor* ignoreActor = nullptr);

	// Produces spawn locations and rotations (relative) based on the provided configuration
	TArray<FTransform> GetSpawnLocations(const FProjectileConfiguration& configuration, const FTransform& rootTransform, UAttackBaseGameplayAbility* spawningAbility);

	void GetRingPoints(const FProjectileConfiguration& configuration, const FTransform& spawnTransform, FTransformArrayA2& returnArray, bool bInwards);

	void GetLinePoints(const FProjectileConfiguration& configuration, FTransformArrayA2& returnArray, FTransform& spawnTransform);

	// Delegate for projectile destruction
	UFUNCTION()
	void OnProjectileDestroyed(AActor* destroyedActor);

	FTransform GetOwnerLookAtTransform();

	TArray<const UCameraComponent*> _ownerCameras;
		
};

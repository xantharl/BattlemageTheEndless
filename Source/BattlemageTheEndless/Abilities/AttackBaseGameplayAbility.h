// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GameplayEffect.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Actor.h"
#include "Engine/EngineTypes.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "Camera/CameraComponent.h"
#include "GA_WithEffectsBase.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEffectRemoved.h"
#include "GameFramework/Character.h"
#include "../Pickups/BattlemageTheEndlessProjectile.h"
#include "../Helpers/Traces.h"
#include "GameplayAbilities/Public/AbilitySystemComponent.h"
#include <Kismet/KismetMathLibrary.h>
#include "HitScanChainEffect.h"
#include "InputTriggers.h"
#include <chrono>
#include "GameplayEffect.h"
#include "HitEffectActor.h"
#include "Components/AudioComponent.h"
#include "AttackBaseGameplayAbility.generated.h"

class UNiagaraSystem;
using namespace std::chrono;

/// <summary>
/// Enum of possible spell hit types
/// </summary>
UENUM(BlueprintType)
enum class HitType : uint8
{
	// For abilities that don't hit anything
	None UMETA(DisplayName = "None"),
	// Buffs or other effects that only affect the caster
	Self UMETA(DisplayName = "Self"),
	// Uses an AnimTrace notify to determine hit
	Melee UMETA(DisplayName = "Melee"),
	// Uses a line trace to determine hit
	HitScan UMETA(DisplayName = "HitScan"),
	// Uses one or more projectiles to determine hit
	Projectile UMETA(DisplayName = "Projectile"),
	// For abilities which are placed at a location rather than casting at a target (e.g. Walls, ground effects, etc.)
	Placed UMETA(DisplayName = "Placed"),
	// Similar to placed but spawns immediately instead of using ghosts with HoldAndRelease
	Actor UMETA(DisplayName = "Actor")
};

/**
 * 
 */
UCLASS()
class BATTLEMAGETHEENDLESS_API UAttackBaseGameplayAbility : public UGA_WithEffectsBase
{
	GENERATED_BODY()

public:
	const float DEFAULT_MAX_DISTANCE = 10000.f;

	// Note the CanEditChange() function is only available when compiling with the editor.
	// Make sure to wrap it with the WITH_EDITOR define or your builds fail!
#if WITH_EDITOR
	virtual bool CanEditChange(const FProperty* InProperty) const override;
#endif
	FTimerHandle TimeoutTimerHandle;

	UAttackBaseGameplayAbility();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = HitBehavior)
	HitType HitType = HitType::None;

	/** Projectile class to spawn */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Projectiles)
	FProjectileConfiguration ProjectileConfiguration;

	/** Required charge duration (s) before ability can be fired **/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ChargeBehavior)
	float ChargeDuration = 0.f;

	/** Current charge duration **/
	milliseconds CurrentChargeDuration;

	/** Curve to control damage of projectile as it charges, X-Axis is MS, Y-Axis is damage **/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = ChargeBehavior)
	UCurveFloat* ChargeDamageCurve;

	/** Curve to control speed of projectile as multiple on projectile's base velocity, X-Axis is MS, Y-Axis is Speed Multiplier **/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = ChargeBehavior)
	UCurveFloat* ChargeVelocityMultiplierCurve;

	/** Curve to control effect of gravity with time charged, X-Axis is MS, Y-Axis is Gravity Multiplier **/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = ChargeBehavior)
	UCurveFloat* ChargeGravityMultiplierCurve;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = ChargeBehavior)
	bool ShouldCastOnPartialCharge = true;

	/** Currently Active Damage for the ability (as of last tick) **/
	UPROPERTY(BlueprintReadOnly, Category = ChargeBehavior)
	float CurrentChargeDamage = 1.f;

	/** Currently Active Projectile Speed Multiple for the ability (as of last tick) **/
	UPROPERTY(BlueprintReadOnly, Category = ChargeBehavior)
	float CurrentChargeProjectileSpeed = 1.f;

	/** Currently Active Gravity Scale Mulitple for the ability (as of last tick) **/
	UPROPERTY(BlueprintReadOnly, Category = ChargeBehavior)
	float CurrentChargeGravityScale = 1.f;

	milliseconds ActivationTime;

	/** Amount of times an ability can chain, 0 = no chaining **/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ChainBehavior)
	int NumberOfChains = 0;

	// Might make this decrease per chain in the future, for now keep it simple
	/** Amount of times an ability can chain, 0 = no chaining **/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ChainBehavior)
	float ChainDistance = 0;

	/** Time in seconds to wait before applying the next chain **/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ChainBehavior)
	float ChainDelay = 0;

	/** Time in seconds to wait before applying the next chain **/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ChainBehavior)
	TObjectPtr<UNiagaraSystem> ChainSystem;

	/** Actor(s) to spawn on hit (e.g. fire surface, explosion, ice wall, etc.) **/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = HitBehavior)
	TArray<TSubclassOf<AHitEffectActor>> HitEffectActors;

	/** Maximum distance for the ability, 0 = unlimited */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Projectile, meta = (AllowPrivateAccess = "true"))
	float MaxRange = DEFAULT_MAX_DISTANCE;

	/** AnimMontage to play during ability charge up */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Animations)
	UAnimMontage* ChargeAnimation;

	/** Sound to play during ability charge up, will be cancelled upon ability release */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sounds)
	USoundWave* ChargeSound;

	/** Sound to play when charge is complete */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sounds)
	USoundWave* ChargeCompleteSound;

	/** Sound to play on cast */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sounds)
	USoundWave* CastSound;

	/** AnimMontage to play when we fire the ability */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Animations)
	UAnimMontage* FireAnimation;

	/** Plays between combo stages if needed (next attack not requested yet) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Animations)
	UAnimMontage* ComboPauseAnimation;

	/// <summary>
	/// override of the ActivateAbility method to apply effects and spawn projectiles
	/// </summary>
	/// <param name="Handle"></param>
	/// <param name="ActorInfo"></param>
	/// <param name="ActivationInfo"></param>
	/// <param name="TriggerEventData"></param>
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData);

	void CreateAndDispatchMontageTask();

	/// <summary>
	/// Chain specific helper to apply effects and spawn projectiles
	/// </summary>
	/// <param name="target"></param>
	/// <param name="targetAsc"></param>
	/// <param name="instigator"></param>
	/// <param name="effectCauser"></param>
	/// <param name="isLastTarget">Controls whether to call endSelf</param>
	void ApplyChainEffects(AActor* target, UAbilitySystemComponent* targetAsc, AActor* instigator, AActor* effectCauser, bool isLastTarget);

	/// <summary>
	/// Helper to apply any effects owned by this ability to the target, which can be the same as the character if applying to self
	/// </summary>
	/// <param name="instigator">Character causing the effect(s)</param>
	/// <param name="target">Target of the effect(s)</param>
	/// <param name="effectCauser">EffectCauser is the actor that is the physical source of the effect</param>
	void ApplyEffects(AActor* target, UAbilitySystemComponent* targetAsc, AActor* instigator = nullptr, AActor* effectCauser = nullptr);

	/// <summary>
	/// Handles any SetByCaller values needed for the effect being applied
	/// </summary>
	/// <param name="effect"></param>
	/// <param name="specHandle"></param>
	/// <param name="effectCauser"></param>
	virtual void HandleSetByCaller(TSubclassOf<UGameplayEffect> effect, FGameplayEffectSpecHandle specHandle, AActor* effectCauser);

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	/// <summary>
	/// Play the animation used for a pause between combo stages if present
	/// </summary>
	/// <param name="ActorInfo"></param>
	void TryPlayComboPause(const FGameplayAbilityActorInfo* ActorInfo);

	UFUNCTION()
	void OnMontageCancelled();

	UFUNCTION()
	void OnMontageCompleted();

	UFUNCTION()
	void OnMontageBlendOut();

	UFUNCTION(BlueprintCallable, Category="ChainBehavior")
	TArray<FTimerHandle>& GetChainTimerHandles() 
	{
		// init if needed
		if (chainTimerHandles.Num() == 0)
		{
			for (int i = 0; i < NumberOfChains; i++)
			{
				chainTimerHandles.Add(FTimerHandle());
			}
		}

		return chainTimerHandles; 
	}

	bool IsCharged();

	virtual void HandleChargeProgress();

	void SpawnHitEffectActors(FHitResult HitResult);
	void SpawnHitEffectActorsAtLocation(FVector Location, FRotator CasterRotation);
	
	/** Called when the ability hits something, damage is handled by the spawning gameplay ability **/
	UFUNCTION()
	void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	void RegisterPlacementGhost(AHitEffectActor* GhostActor);
	TArray<TObjectPtr<AHitEffectActor>> GetPlacementGhosts() const { return _placementGhosts; }
	void ClearPlacementGhosts() { _placementGhosts.Empty(); }

	UFUNCTION()
	void OnPlacementGhostDestroyed(AActor* PlacementGhost);

	void SpawnSpellActors(bool isGhost, bool attachToCharacter = false);

	void PositionSpellActor(AHitEffectActor* hitEffectActor, ACharacter* character);

	UFUNCTION(BlueprintCallable, Category = "Projectile Physics")
	FRotator CalculateAttackAngle(FVector StartLocation, FVector TargetLocation, bool bAssumeFullCharge = true);

private:
	/** Timer handles for chained abilities, do not reference directly without ensuring it is initialized
		See GetChainTimerHandles **/
	TArray<FTimerHandle> chainTimerHandles;

	bool _bIsCharged = false;

	FTimerHandle ChargeCompleteSoundTimerHandle;

	UAudioComponent* ChargeSoundComponent;

	// Used to track placement ghosts, which are actors that are used to preview the placement of an ability
	TArray<TObjectPtr<AHitEffectActor>> _placementGhosts;

	void PlayChargeCompleteSound();

	float GetEffectiveProjectileGravity(ABattlemageTheEndlessProjectile* defaultProjectile, bool bAssumeFullCharge);
	float GetEffectiveProjectileSpeed(ABattlemageTheEndlessProjectile* defaultProjectile, bool bAssumeFullCharge);
};

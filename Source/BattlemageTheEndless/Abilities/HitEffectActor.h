// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Abilities/GameplayAbility.h"
#include "Components/SphereComponent.h"
#include "NiagaraSystem.h"
#include "NiagaraComponent.h"
#include <NiagaraFunctionLibrary.h>
#include "AbilitySystemComponent.h"
#include "HitEffectActor.generated.h"

UENUM(BlueprintType)
enum class EOnApplyEffectsBehavior : uint8
{
	None UMETA(DisplayName = "None"),
	DestroyComponent UMETA(DisplayName = "DestroyComponent"),
	DestroyActor UMETA(DisplayName = "DestroyActor")
};

/// <summary>
/// AHitEffectActor is a simple actor that is spawned when a hit is detected by an ability. It is used primarily to spawn persistent volumes.
/// </summary>
UCLASS()
class BATTLEMAGETHEENDLESS_API AHitEffectActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AHitEffectActor();

	virtual void InitCollisionObject();
	virtual void InitCollisionType();

	/** Snaps actor to ground if SnapToGround is true **/
	void SnapActorToGround(FHitResult hitResult);

	virtual void BeginPlay();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hit Effect")
	bool SnapToGround = true;

	/** Material to be used during placement ghost phase, this MUST BE a material instance of the main material **/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hit Effect")
	UMaterialInstance* PlacementGhostMaterial;

	/** Store a reference to the spawning ability in case we need details **/
	UGameplayAbility* SpawningAbility;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hit Effect")
	TArray<TSubclassOf<UGameplayEffect>> Effects;

	/** The interval at which the effect is re-applied, 0 = only applied once without exiting and re-entering **/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hit Effect")
	float EffectApplicationInterval = 0.5f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "Hit Effect")
	TObjectPtr<UShapeComponent> AreaOfEffect;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hit Effect")
	TObjectPtr<UNiagaraSystem> VisualEffectSystem;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hit Effect")
	FVector VisualEffectScale = FVector::OneVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hit Effect")
	bool bRootShouldApplyEffects = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hit Effect")
	EOnApplyEffectsBehavior OnApplyEffectsBehavior = EOnApplyEffectsBehavior::None;

	/** Some abilities have different behavior based on active tags, this actor will only spawn if all required tags are met **/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hit Effect")
	FGameplayTagContainer RequiredTags;

	UFUNCTION(BlueprintCallable, Category = "Hit Effect")
	virtual void OnAreaOfEffectBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION(BlueprintCallable, Category = "Hit Effect")
	virtual void OnAreaOfEffectEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	TObjectPtr<AActor> Instigator;

	void ActivateEffect(TObjectPtr<UNiagaraSystem> system);

	void DeactivateEffect();

	virtual void Destroyed() override;

private:
	virtual bool Validate();

	UNiagaraComponent* _visualEffectInstance;

	TMap<AActor*, FTimerHandle> _effectReapplyTimers;

	virtual void ApplyEffects(AActor* actor, UPrimitiveComponent* applyingComponent);
};

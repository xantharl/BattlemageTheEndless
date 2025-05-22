// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayCueNotify_Actor.h"
#include <NiagaraSystem.h>
#include <NiagaraComponent.h>
#include <NiagaraFunctionLibrary.h>
#include "GameFramework/Character.h"
#include "GameplayCueNotify_ActorWNiagara.generated.h"

UENUM()
enum class EAttachType : uint8
{
	None UMETA(DisplayName = "None"),
	Character UMETA(DisplayName = "Character"),
	PrimaryWeapon UMETA(DisplayName = "PrimaryWeapon"),
	SecondaryWeapon UMETA(DisplayName = "SecondaryWeapon")
};

USTRUCT(BlueprintType)
struct FAttackEffectData
{
	GENERATED_BODY()

	/// <summary>
	/// Determines whether the effect should spawn at the offset location or on the ground below the offset location
	/// </summary>
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Effects, meta = (AllowPrivateAccess = "true"))
	bool bSnapToGround = false;

	/// <summary>
	/// Determines which object to attach to, if any
	/// </summary>
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Effects, meta = (AllowPrivateAccess = "true"))
	EAttachType AttachType = EAttachType::None;

	/// <summary>
	/// Specifies the attachment point for the effect, ignored if attach type is none
	/// </summary>
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Effects, meta = (AllowPrivateAccess = "true"))
	FName AttachSocket;

	/// <summary>
	/// Determines whether the effect should kill the previous effect
	///		This is handled in UAbilityComboManager::ActivateAbilityAndResetTimer
	/// </summary>
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Effects, meta = (AllowPrivateAccess = "true"))
	bool bShouldKillPreviousEffect = false;
};

/**
 * 
 */
UCLASS()
class BATTLEMAGETHEENDLESS_API AGameplayCueNotify_ActorWNiagara : public AGameplayCueNotify_Actor
{
	GENERATED_BODY()

public:
	AGameplayCueNotify_ActorWNiagara();

	/// <summary>
	/// This component is necessary so that we can move the attached child components relative to it
	/// </summary>
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Niagara, meta = (AllowPrivate))
	USceneComponent* SceneRoot;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Niagara, meta = (AllowPrivate))
	UNiagaraComponent* ParticleSystem;

	// CURRENTLY UNUSED, Particle System handles config and spawning 
	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Niagara, meta = (AllowPrivate))
	//TObjectPtr<UNiagaraComponent> NiagaraInstance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Effects, meta = (AllowPrivate))
	FAttackEffectData AttackEffect;

	virtual void HandleGameplayCue(AActor* MyTarget, EGameplayCueEvent::Type EventType, const FGameplayCueParameters& Parameters) override;

	void TryDestroyNiagaraInstance();

	FTimerHandle DestroyTimer;
	
protected:
	//void TryCreateNiagaraInstance(AActor* MyTarget);

	UFUNCTION()
	void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);
};

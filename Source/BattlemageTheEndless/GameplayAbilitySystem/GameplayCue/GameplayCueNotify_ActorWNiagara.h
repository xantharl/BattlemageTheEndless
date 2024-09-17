// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayCueNotify_Actor.h"
#include <NiagaraSystem.h>
#include <NiagaraComponent.h>
#include <NiagaraFunctionLibrary.h>
#include "GameFramework/Character.h"
#include "GameplayCueNotify_ActorWNiagara.generated.h"

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
	/// Optional attachment socket if the effect should be attached to the character
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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Niagara, meta = (AllowPrivate))
	UNiagaraComponent* NiagaraConfig;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Niagara, meta = (AllowPrivate))
	UNiagaraComponent* NiagaraInstance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Effects, meta = (AllowPrivate))
	FAttackEffectData AttackEffect;
	virtual void TickActor(float DeltaTime, enum ELevelTick TickType, FActorTickFunction& ThisTickFunction) override;
};

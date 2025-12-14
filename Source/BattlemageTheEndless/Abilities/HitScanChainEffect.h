// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include <NiagaraComponent.h>
#include <NiagaraSystem.h>
#include <NiagaraFunctionLibrary.h>
#include <NiagaraDataInterfaceArrayFunctionLibrary.h>
#include "HitScanChainEffect.generated.h"

UCLASS()
class BATTLEMAGETHEENDLESS_API AHitScanChainEffect : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AHitScanChainEffect();

	/** Effect class to spawn **/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
	TObjectPtr<UNiagaraSystem> HitScanChainEffect;

	/// <summary>
	/// Initializes the hitscan chain effect actor with the necessary data to spawn and manage the effect
	/// </summary>
	/// @param originatingActor Actor where the chain originates, e.g. the caster
	/// @param targetActor Actor being hit by the chain
	/// @param hitScanChainEffect Niagara system to use for the effect
	/// @param beamEnd Optional Override for beam end location instead of using target actor location
	void Init(TObjectPtr<AActor> originatingActor, TObjectPtr<AActor> targetActor, TObjectPtr<UNiagaraSystem> hitScanChainEffect, FVector beamEnd = FVector::ZeroVector);

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnSystemFinished(UNiagaraComponent* finishedComponent);

	/** Spawned instance of effect class **/
	TObjectPtr<UNiagaraComponent> HitScanChainEffectInstance;

	/** Actor to anchor to on start of chain **/
	TObjectPtr<AActor> OriginatingActor;

	/** Actor to anchor to on end of chain **/
	TObjectPtr<AActor> TargetActor;

	/** Override for Actor Location based beam end **/
	FVector BeamEnd;
public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
};

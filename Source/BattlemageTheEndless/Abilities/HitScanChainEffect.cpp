// Fill out your copyright notice in the Description page of Project Settings.


#include "HitScanChainEffect.h"

// Sets default values
AHitScanChainEffect::AHitScanChainEffect()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

void AHitScanChainEffect::Init(TObjectPtr<AActor> originatingActor, TObjectPtr<AActor> targetActor, TObjectPtr<UNiagaraSystem> hitScanChainEffect)
{
	HitScanChainEffect = hitScanChainEffect;
	OriginatingActor = originatingActor;
	TargetActor = targetActor;

	// Spawn the system and subscribe
	HitScanChainEffectInstance = UNiagaraFunctionLibrary::SpawnSystemAtLocation(OriginatingActor, HitScanChainEffect, OriginatingActor->GetActorLocation());
	HitScanChainEffectInstance->OnSystemFinished.AddDynamic(this, &AHitScanChainEffect::OnSystemFinished);

	// Set the end location of the beam
	HitScanChainEffectInstance->SetVariablePosition(FName("BeamEnd"), OriginatingActor->GetActorLocation());
}

// Called when the game starts or when spawned
void AHitScanChainEffect::BeginPlay()
{
	Super::BeginPlay();
	
}

void AHitScanChainEffect::OnSystemFinished(UNiagaraComponent* finishedComponent)
{
	Destroy();
}

// Called every frame
void AHitScanChainEffect::Tick(float DeltaTime)
{
	HitScanChainEffectInstance->SetWorldLocation(OriginatingActor->GetActorLocation());
	HitScanChainEffectInstance->SetVariablePosition(FName("BeamEnd"), TargetActor->GetActorLocation());
	Super::Tick(DeltaTime);
}


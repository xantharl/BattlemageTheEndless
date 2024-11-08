// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HitEffectActor.generated.h"

// TODO: This class may not be necessary, turns out AActor has a built-in InitialLifeSpan feature
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

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	/// <summary>
	/// Lifetime of the actor in seconds, will automatically be destroyed after this time relative to World->GetTimeSeconds() and CreationTime.
	/// A Lifetime of 0 means unlimited, but this is discouraged as the actor will never be cleaned up.
	/// </summary>
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Configuration)
	float Lifetime = 1.f;
};

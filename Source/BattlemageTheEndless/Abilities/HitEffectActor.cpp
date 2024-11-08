// Fill out your copyright notice in the Description page of Project Settings.


#include "HitEffectActor.h"

// Sets default values
AHitEffectActor::AHitEffectActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AHitEffectActor::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AHitEffectActor::Tick(float DeltaTime)
{
	// Check if we need to destroy this actor
	auto world = GetWorld();
	if (Lifetime > 0.f && world && world->GetTimeSeconds() - CreationTime > Lifetime)
	{
		Destroy();
		// no need to execute super, we're destroying the actor
		return;
	}
	Super::Tick(DeltaTime);
}


// Fill out your copyright notice in the Description page of Project Settings.


#include "MyHitEffectActor_BoxCollision.h"

void AMyHitEffectActor_BoxCollision::InitCollisionObject()
{
	PrimaryActorTick.bCanEverTick = true;

	// Create a box collision component
	auto area = CreateDefaultSubobject<UBoxComponent>(TEXT("AreaOfEffect"));
	area->SetBoxExtent(FVector(50.f, 50.f, 50.f));
	AreaOfEffect = area;
	SetRootComponent(AreaOfEffect);
}
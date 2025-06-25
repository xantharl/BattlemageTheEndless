// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/HitEffectActor.h"
#include "Components/BoxComponent.h"
#include "MyHitEffectActor_BoxCollision.generated.h"

// TODO: Remove this class
/**
 * 
 */
UCLASS()
class BATTLEMAGETHEENDLESS_API AMyHitEffectActor_BoxCollision : public AHitEffectActor
{
	GENERATED_BODY()

public:
	void InitCollisionObject() override;
};

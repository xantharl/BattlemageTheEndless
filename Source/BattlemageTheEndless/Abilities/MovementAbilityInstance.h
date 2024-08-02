// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MovementAbilityInstance.generated.h"

/**
 * 
 */
UCLASS()
class BATTLEMAGETHEENDLESS_API UMovementAbilityInstance: public UObject
{
	GENERATED_BODY()
public:
	UMovementAbilityInstance(const FObjectInitializer& X);
	~UMovementAbilityInstance();

	bool IsActive = false;

	virtual void Begin() { IsActive = true; }
	virtual void End() { IsActive = false; }

	virtual void Tick(float DeltaTime);
};

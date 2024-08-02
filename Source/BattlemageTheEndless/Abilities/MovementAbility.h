// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MovementAbilityInstance.h"
#include "MovementAbility.generated.h"

UENUM()
enum class MovementAbilityType : uint8
{
	Slide UMETA(DisplayName = "Slide"),
	Launch UMETA(DisplayName = "Launch"),
	WallRun UMETA(DisplayName = "WallRun"),
	Vault UMETA(DisplayName = "Vault")
};

/**
 * This class is intended to be used as a base class for all movement abilities which have a ticking element
 
 */
UCLASS(Abstract)
class BATTLEMAGETHEENDLESS_API UMovementAbility : public UObject
{
	GENERATED_BODY()
public:
	UMovementAbility(const FObjectInitializer& X);
	UMovementAbility() {}
	~UMovementAbility();

	bool IsEnabled = true;

	// Lower values are executed first
	int Priority = 0;

	MovementAbilityType Type;

	bool IsActive = false;

	virtual void Begin() { IsActive = true; }
	virtual void End() { IsActive = false; }

	virtual void Tick(float DeltaTime);
};
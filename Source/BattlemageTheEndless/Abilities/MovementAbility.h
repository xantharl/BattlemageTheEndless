// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MovementAbilityInstance.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "MovementAbility.generated.h"

UENUM(BlueprintType)
enum class MovementAbilityType: uint8
{
	None UMETA(DisplayName = "None"),
	Sprint UMETA(DisplayName = "Sprint"),
	Slide UMETA(DisplayName = "Slide"),
	Launch UMETA(DisplayName = "Launch"),
	WallRun UMETA(DisplayName = "WallRun"),
	Vault UMETA(DisplayName = "Vault"),
	Dodge UMETA(DisplayName = "Dodge")
};

/**
 * This class is intended to be used as a base class for all movement abilities which have a ticking element
 */
DECLARE_DYNAMIC_MULTICAST_SPARSE_DELEGATE_OneParam(FMovementAbilityBeginSignature, UMovementAbility, OnMovementAbilityBegin, UMovementAbility*, MovementAbility);
DECLARE_DYNAMIC_MULTICAST_SPARSE_DELEGATE_OneParam(FMovementAbilityEndSignature, UMovementAbility, OnMovementAbilityEnd, UMovementAbility*, MovementAbility);
UCLASS(Abstract)
class BATTLEMAGETHEENDLESS_API UMovementAbility : public UObject
{
	GENERATED_BODY()
protected:
	ACharacter* Character;
	USkeletalMeshComponent* Mesh;
	UCharacterMovementComponent* Movement;

public:
	UMovementAbility(const FObjectInitializer& X);
	//UMovementAbility() {}
	~UMovementAbility();

	// Determines whether a character can use this ability
	bool IsEnabled = true;

	// Determines whether the ability is currently active
	UPROPERTY(BlueprintReadOnly)
	bool IsActive = false;

	// Lower values are executed first
	int Priority = 10;

	UPROPERTY(BlueprintReadOnly)
	MovementAbilityType Type;

	virtual void Init(UCharacterMovementComponent* movement, ACharacter* character, USkeletalMeshComponent* mesh);
	virtual bool ShouldBegin() { return true; }
	virtual bool ShouldEnd() { return true; }
	virtual void Begin();
	virtual void End();
	virtual void Tick(float DeltaTime) {}


	FMovementAbilityBeginSignature OnMovementAbilityBegin;
	FMovementAbilityEndSignature OnMovementAbilityEnd;
};
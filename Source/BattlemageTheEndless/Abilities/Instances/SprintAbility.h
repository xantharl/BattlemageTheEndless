// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "../MovementAbility.h"
#include "SprintAbility.generated.h"

/**
 * 
 */
UCLASS()
class BATTLEMAGETHEENDLESS_API USprintAbility : public UMovementAbility
{
	GENERATED_BODY()
	
	virtual void Begin(const FGameplayEventData* TriggerEventData = nullptr) override;

public:
	USprintAbility(const FObjectInitializer& X);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CharacterMovement, meta = (AllowPrivateAccess = "true"))
	float SprintSpeed = 1200.f;
};

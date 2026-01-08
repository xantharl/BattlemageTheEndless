// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "../MovementAbility.h"
#include "GameFramework/Character.h"
#include "LaunchAbility.generated.h"

/**
 * 
 */
UCLASS()
class BATTLEMAGETHEENDLESS_API ULaunchAbility : public UMovementAbility
{
	GENERATED_BODY()

	ULaunchAbility(const FObjectInitializer& X);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CharacterMovement, meta = (AllowPrivateAccess = "true"))
	FVector LaunchImpulse = FVector(400.f, 0.f, 850.f);

	virtual void Begin(const FGameplayEventData* TriggerEventData = nullptr) override;
	virtual bool ShouldBegin() override;
};

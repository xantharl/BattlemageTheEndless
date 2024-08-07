// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "../MovementAbility.h"
#include "SlideAbility.generated.h"

/**
 * 
 */
UCLASS()
class BATTLEMAGETHEENDLESS_API USlideAbility : public UMovementAbility
{
	GENERATED_BODY()

	USlideAbility(const FObjectInitializer& X);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CharacterMovement, meta = (AllowPrivateAccess = "true"))
	float SlideSpeed = 1200.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CharacterMovement, meta = (AllowPrivateAccess = "true"))
	float SlideHalfHeight = 35.0f;

	// The default value is based on the slide animation length
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CharacterMovement, meta = (AllowPrivateAccess = "true"))
	float SlideDurationSeconds = 1.67f;
	float SlideElapsedSeconds = 0.f;
	
	void Begin() override;
	void End() override;
	void Tick(float DeltaTime) override;
};

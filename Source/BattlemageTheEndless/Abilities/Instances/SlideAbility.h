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

	/** Time elapsed as computed by delta time, sliding downhill decrements this. A Negative value accelerates the player **/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CharacterMovement, meta = (AllowPrivateAccess = "true"))
	float SlideElapsedSeconds = 0.f;

	/** Threshold of negative Z Axis speed required to gain momentum while sliding **/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CharacterMovement, meta = (AllowPrivateAccess = "true"))
	float SlideAccelerationThreshold = -2.f;

	/** Rate at which sliding downhill accelerates the player (cm/s^2) **/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CharacterMovement, meta = (AllowPrivateAccess = "true"))
	float SlideAccelerationRate = 400.f;

	/** Rate at which sliding downhill accelerates the player (cm/s^2) **/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CharacterMovement, meta = (AllowPrivateAccess = "true"))
	float SlideDeccelerationRate = 600.f;
	
	void Begin() override;
	void End(bool bForce = false) override;
	void Tick(float DeltaTime) override;
};

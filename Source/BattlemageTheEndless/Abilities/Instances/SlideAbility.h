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

public:
	USlideAbility(const FObjectInitializer& X);

	/** Angle of slide for use by character to set pitch **/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CharacterMovement, meta = (AllowPrivateAccess = "true"))
	float LastActualAngle = 0.f;

	/** Rate at which the character will accelerate on transition out as needed. This is constrained by a cap dependent on whether the player is crouched **/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CharacterMovement, meta = (AllowPrivateAccess = "true"))
	float TransitionOutAccelertaionRate = 1200.f;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CharacterMovement, meta = (AllowPrivateAccess = "true"))
	float SlideSpeed = 1200.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CharacterMovement, meta = (AllowPrivateAccess = "true"))
	float SlideHalfHeight = 35.0f;

	// The default value is based on the slide animation length
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CharacterMovement, meta = (AllowPrivateAccess = "true"))
	float SlideDurationSeconds = 1.67f;

	/** Threshold of negative Z Axis speed required to gain momentum while sliding **/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CharacterMovement, meta = (AllowPrivateAccess = "true"))
	float SlideAccelerationThreshold = -2.f;

	/** Angle at which we hit max accel/decel. Value is expected to be between 0 and 90 **/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CharacterMovement, meta = (AllowPrivateAccess = "true"))
	float AccelerationLimitAngle = 45.f;

	/** Max Rate at which sliding downhill accelerates the player (cm/s^2). Max rate is applied at AccelerationLimitAngle degrees or greater. **/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CharacterMovement, meta = (AllowPrivateAccess = "true"))
	float MaxSlideAccelerationRate = 400.f;

	/** Base Rate at which sliding decelerates the player (cm/s^2). Max rate is applied at AccelerationLimitAngle degrees or greater. **/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CharacterMovement, meta = (AllowPrivateAccess = "true"))
	float BaseSlideDecelerationRate = 600.f;

	/** Max Rate at which sliding uphill decelerates the player (cm/s^2). Max rate is applied at AccelerationLimitAngle degrees or greater. **/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CharacterMovement, meta = (AllowPrivateAccess = "true"))
	float MaxSlideDecelerationRate = 1200.f;
	
	void Begin() override;
	void End(bool bForce = false) override;
	void Tick(float DeltaTime) override;

private:
	FVector _previousLocation;
};

// Fill out your copyright notice in the Description page of Project Settings.


#include "SlideAbility.h"
USlideAbility::USlideAbility(const FObjectInitializer& X) : Super(X)
{
	Type = MovementAbilityType::Slide;
	TransitionInDuration = 0.5f;
	TransitionOutDuration = 0.7f;
	// Make this ability override sprint
	Priority = 2;
}

void USlideAbility::Begin()
{
	Super::Begin();
	Movement->SetCrouchedHalfHeight(SlideHalfHeight);
	_previousLocation = Movement->GetActorLocation();
	_priorRotationRateZ = Movement->RotationRate.Yaw;
	Movement->bUseControllerDesiredRotation = true;
	Character->bUseControllerRotationYaw = false;
 	Movement->RotationRate.Yaw = RotationRateZ;
}

void USlideAbility::End(bool bForce)
{
	// Conserve momentum if we're going fast enough, will be reset on OnMovementModeChanges in character
	if (Movement->MaxWalkSpeedCrouched > Movement->MaxWalkSpeed) 
	{
		Movement->MaxWalkSpeed = Movement->MaxWalkSpeedCrouched;
	}
	Movement->bUseControllerDesiredRotation = false;
	Character->bUseControllerRotationYaw = true;
	Movement->RotationRate.Yaw = _priorRotationRateZ;
	Super::End(bForce);
}

void USlideAbility::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	FVector currentLocation = Movement->GetActorLocation();

	// Don't count time till we're done transitioning in
	if (isTransitioningIn)
	{
		_previousLocation = currentLocation;
		return;
	}

	// check if we're on a downslope
	FVector moved = currentLocation - _previousLocation;
	moved.Normalize();

	bool decelerating = true;
	// "refund" elapsed time if sliding downhill
	if (moved.Z < -0.0001f)
	{
		elapsed -= round<milliseconds>(duration<float>{DeltaTime * 1000.f});
		decelerating = false;
	}


	// If we've reached the slide's full duration, end the slide
	if (elapsed >= round<milliseconds>(duration<float>{SlideDurationSeconds * 1000.f}))
	{
		_previousLocation = currentLocation;
		End();
		return;
	}

	// Default the effective angle to 0 to
	float effectiveAngle = 0.f;
	float currentPitch = moved.Rotation().Pitch;
	LastActualAngle = currentPitch;

	// Calculate the correct angle if we're on a slope
	if (FMath::Abs(currentPitch) > 0.0001f)
		// Cap the angle to 45 degrees for calculating acceleration
		effectiveAngle = FMath::Min(AccelerationLimitAngle, FMath::Abs(currentPitch));

	float deltaSpeed;
	// if we're going uphill, decelerate at a rate of BaseSlideDecelerationRate + appropriate percentage of difference between base and max
	if (decelerating)
		deltaSpeed = DeltaTime * -(BaseSlideDecelerationRate + (MaxSlideDecelerationRate - BaseSlideDecelerationRate) * (effectiveAngle / AccelerationLimitAngle));
	// if we're going downhill, accelerate at a rate of MaxSlideAccelerationRate * appropriate percentage of slope
	else
		deltaSpeed = DeltaTime * (MaxSlideAccelerationRate * (effectiveAngle / AccelerationLimitAngle));


	Movement->MaxWalkSpeedCrouched = FMath::Max(Movement->MaxWalkSpeedCrouched + deltaSpeed, 0.f);
	// End if we've hit 0 speed
	if (FMath::Abs(Movement->MaxWalkSpeedCrouched) < 0.0001f)
	{
		_previousLocation = currentLocation;
		End();
		return;
	}

	_previousLocation = currentLocation;
}

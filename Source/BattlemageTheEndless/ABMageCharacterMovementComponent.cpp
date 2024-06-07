// Fill out your copyright notice in the Description page of Project Settings.


#include "ABMageCharacterMovementComponent.h"

FRotator UABMageCharacterMovementComponent::ComputeOrientToMovementRotation(const FRotator& CurrentRotation, float DeltaTime, FRotator& DeltaRotation) const
{
	if (Acceleration.SizeSquared() < 0.1)
	{
		return CurrentRotation;
	}

	// Rotate toward direction of acceleration.
	return Acceleration.GetSafeNormal().Rotation();
	//return Acceleration.GetSafeNormal().RotateAngleAxis(180, FVector(0, 0, 1)).Rotation();
}
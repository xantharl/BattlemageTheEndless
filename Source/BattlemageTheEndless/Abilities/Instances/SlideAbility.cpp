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
}

void USlideAbility::End(bool bForce)
{
	SlideElapsedSeconds = 0.0f;
	Super::End(bForce);
}

void USlideAbility::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Don't count time till we're done transitioning in
	if (isTransitioningIn)
	{
		return;
	}

	// check if we're on a downslope
	if (Movement->Velocity.Z < SlideAccelerationThreshold)
	{
		SlideElapsedSeconds -= DeltaTime;
	}
	else
	{
		SlideElapsedSeconds += DeltaTime;
	}


	// If we've reached the slide's full duration, reset speed and slide data
	if (SlideElapsedSeconds >= SlideDurationSeconds)
	{
		End();
		return;
	}

	if (SlideElapsedSeconds >= 0.f)
	{
		// Otherwise decrement the slide speed	
		Movement->MaxWalkSpeedCrouched = SlideSpeed - (SlideElapsedSeconds * SlideDeccelerationRate);
	}
	else
	{
		// Accelerate the player
		Movement->MaxWalkSpeedCrouched = SlideSpeed + (SlideElapsedSeconds * SlideAccelerationRate);
	}
}

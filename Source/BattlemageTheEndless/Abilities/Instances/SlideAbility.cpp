// Fill out your copyright notice in the Description page of Project Settings.


#include "SlideAbility.h"

void USlideAbility::Begin()
{
	UMovementAbility::Begin();
	Movement->SetCrouchedHalfHeight(SlideHalfHeight);
}

void USlideAbility::End()
{
	UMovementAbility::End();
	SlideElapsedSeconds = 0.0f;
}

void USlideAbility::Tick(float DeltaTime)
{
	SlideElapsedSeconds += DeltaTime;

	// If we've reached the slide's full duration, reset speed and slide data
	if (SlideElapsedSeconds >= SlideDurationSeconds)
	{
		End();
		return;
	}

	// Otherwise decrement the slide speed	
	float newSpeed = SlideSpeed * powf(2.7182818284f, -1.39f * SlideElapsedSeconds);
	Movement->MaxWalkSpeedCrouched = newSpeed;
}

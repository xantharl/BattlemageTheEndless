// Fill out your copyright notice in the Description page of Project Settings.


#include "SlideAbility.h"
USlideAbility::USlideAbility(const FObjectInitializer& X) : Super(X)
{
	Type = MovementAbilityType::Slide;
	// Make this ability override sprint
	Priority = 2;
}

void USlideAbility::Begin()
{
	Super::Begin();
	Movement->SetCrouchedHalfHeight(SlideHalfHeight);
}

void USlideAbility::End()
{
	SlideElapsedSeconds = 0.0f;
	Super::End();
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

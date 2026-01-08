// Fill out your copyright notice in the Description page of Project Settings.


#include "LaunchAbility.h"

ULaunchAbility::ULaunchAbility(const FObjectInitializer& X) : Super(X)
{
	Type = MovementAbilityType::Launch;
	// Make this ability override just about everything
	Priority = 1;
}

void ULaunchAbility::Begin(const FGameplayEventData* TriggerEventData)
{
	Super::Begin();
	Type = MovementAbilityType::Launch;

	// Create the launch vector
	FRotator rotator = Character->GetActorRotation();
	rotator.Pitch = 0;
	FVector vector = LaunchImpulse.RotateAngleAxis(rotator.Yaw, FVector::ZAxisVector);

	Character->LaunchCharacter(vector, false, true);

	// This ability is immediate, so there's no need to keep it active
	End();
}

bool ULaunchAbility::ShouldBegin()
{
	// also requires sprint but we have to handle that in the movement component
	return Movement->IsCrouching();
}
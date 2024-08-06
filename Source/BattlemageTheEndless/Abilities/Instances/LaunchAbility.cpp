// Fill out your copyright notice in the Description page of Project Settings.


#include "LaunchAbility.h"

void ULaunchAbility::Begin()
{
	OnMovementAbilityBegin.Broadcast(this);

	// Create the launch vector
	FRotator rotator = Character->GetActorRotation();
	rotator.Pitch = 0;
	FVector vector = LaunchImpulse.RotateAngleAxis(rotator.Yaw, FVector::ZAxisVector);

	Character->LaunchCharacter(vector, false, true);
}

void ULaunchAbility::End()
{
}

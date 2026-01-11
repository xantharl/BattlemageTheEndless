// Fill out your copyright notice in the Description page of Project Settings.
#include "SprintAbility.h"

USprintAbility::USprintAbility(const FObjectInitializer& X) : Super(X)
{
	Type = MovementAbilityType::Sprint;
	// sprint will be overridden by most things
	Priority = 5;
}

void USprintAbility::Begin(const FMovementEventData& MovementEventData)
{
	UMovementAbility::Begin(MovementEventData);
	Movement->MaxWalkSpeed = SprintSpeed;
}
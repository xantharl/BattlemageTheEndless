// Fill out your copyright notice in the Description page of Project Settings.


#include "MovementAbility.h"

UMovementAbility::UMovementAbility(const FObjectInitializer& X, UCharacterMovementComponent* Movement) : UObject(X)
{
	this->Movement = Movement;
}

UMovementAbility::~UMovementAbility()
{
}

// Fill out your copyright notice in the Description page of Project Settings.


#include "MovementAbility.h"

UMovementAbility::UMovementAbility(const FObjectInitializer& X) : UObject(X)
{
}

UMovementAbility::~UMovementAbility()
{
}

void UMovementAbility::Init(UCharacterMovementComponent* movement, ACharacter* character, USkeletalMeshComponent* mesh)
{
	Movement = movement; 
	Character = character; 
	Mesh = mesh;
}

void UMovementAbility::Begin()
{
	IsActive = true;
	OnMovementAbilityBegin.Broadcast(this);
}

void UMovementAbility::End()
{
	IsActive = false;
	OnMovementAbilityEnd.Broadcast(this);
}

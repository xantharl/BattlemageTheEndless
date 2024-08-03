// Fill out your copyright notice in the Description page of Project Settings.


#include "MovementAbility.h"

UMovementAbility::UMovementAbility(const FObjectInitializer& X) : UObject(X)
{
}

UMovementAbility::~UMovementAbility()
{
}

void UMovementAbility::Init(UCharacterMovementComponent* movement, AActor* character, USkeletalMeshComponent* mesh)
{
	Movement = movement; 
	Character = character; 
	Mesh = mesh;
}

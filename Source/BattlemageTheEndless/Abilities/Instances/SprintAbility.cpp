// Fill out your copyright notice in the Description page of Project Settings.


#include "SprintAbility.h"

void USprintAbility::Begin()
{
	Super::Begin();
	Movement->MaxWalkSpeed = SprintSpeed;
}

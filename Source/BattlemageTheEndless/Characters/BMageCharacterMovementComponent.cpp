// Fill out your copyright notice in the Description page of Project Settings.


#include "BMageCharacterMovementComponent.h"

UBMageCharacterMovementComponent::UBMageCharacterMovementComponent()
{
	MovementAbilities = map<MovementAbilityType, UMovementAbility*>();

	MovementAbilities[MovementAbilityType::WallRun] = CreateDefaultSubobject<UWallRunAbility>(TEXT("WallRun"));
	MovementAbilities[MovementAbilityType::Launch] = CreateDefaultSubobject<UWallRunAbility>(TEXT("Launch"));
	MovementAbilities[MovementAbilityType::Slide] = CreateDefaultSubobject<UWallRunAbility>(TEXT("Slide"));
	MovementAbilities[MovementAbilityType::Vault] = CreateDefaultSubobject<UWallRunAbility>(TEXT("Vault"));

	AirControl = BaseAirControl;
	GetNavAgentPropertiesRef().bCanCrouch = true;
}

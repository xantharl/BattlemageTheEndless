// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include <map>
#include <string>
//#include "../Abilities/MovementAbility.h"
#include "BMageCharacterMovementComponent.generated.h"

using namespace std;
/**
 * 
 */
UCLASS()
class BATTLEMAGETHEENDLESS_API UBMageCharacterMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()	

public:
	UBMageCharacterMovementComponent();
	//map<string, UMovementAbility> MovementAbilities;

};

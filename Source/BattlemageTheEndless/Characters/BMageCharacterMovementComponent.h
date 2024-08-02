// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include <map>
#include <string>

#include "../Abilities/MovementAbility.h"
#include "../Abilities/Instances/WallRunAbility.h"
#include "../Abilities/Instances/VaultAbility.h"
#include "../Abilities/Instances/SlideAbility.h"
#include "../Abilities/Instances/LaunchAbility.h"

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
	map<MovementAbilityType, UMovementAbility*> MovementAbilities;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CharacterMovement, meta = (AllowPrivateAccess = "true"))
	float BaseAirControl = 0.8f;
};

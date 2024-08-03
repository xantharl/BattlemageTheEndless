// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "../MovementAbility.h"
#include "SlideAbility.generated.h"

/**
 * 
 */
UCLASS()
class BATTLEMAGETHEENDLESS_API USlideAbility : public UMovementAbility
{
	GENERATED_BODY()

	USlideAbility(const FObjectInitializer& X) : Super(X) { Type = MovementAbilityType::Slide; }
	
};

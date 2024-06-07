// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "ABMageCharacterMovementComponent.generated.h"

/**
 * 
 */
UCLASS()
class BATTLEMAGETHEENDLESS_API UABMageCharacterMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()

	FRotator ComputeOrientToMovementRotation(const FRotator& CurrentRotation, float DeltaTime, FRotator& DeltaRotation) const;
	
};

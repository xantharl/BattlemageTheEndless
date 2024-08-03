// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "../MovementAbility.h"
#include "VaultAbility.generated.h"

/**
 * 
 */
UCLASS()
class BATTLEMAGETHEENDLESS_API UVaultAbility : public UMovementAbility
{
	GENERATED_BODY()

	UVaultAbility(const FObjectInitializer& X) : Super(X) { Type = MovementAbilityType::Vault; }
	
};

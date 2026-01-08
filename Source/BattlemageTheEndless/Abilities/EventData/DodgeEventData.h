// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayAbilitiesEditor/Public/GameplayAbilityAudit.h"
#include "UObject/Object.h"
#include "DodgeEventData.generated.h"

/**
 * 
 */
UCLASS()
class BATTLEMAGETHEENDLESS_API UDodgeEventData : public UObject
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Dodge )
	FVector DodgeInputVector;
	
	virtual bool IsSupportedForNetworking() const override { return true; };
};

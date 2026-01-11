// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GameFramework/PlayerController.h"
#include "BattlemageTheEndless/Abilities/MovementAbility.h"
#include "BattlemageTheEndlessPlayerController.generated.h"

class UInputMappingContext;

/**
 *
 */
UCLASS()
class BATTLEMAGETHEENDLESS_API ABattlemageTheEndlessPlayerController : public APlayerController
{
	GENERATED_BODY()
	
public:	
	UFUNCTION(Server, Reliable)
	void Server_HandleMovementEvent(const FGameplayTag EventTag, const FMovementEventData& MovementEventData);
	
protected:

	/** Input Mapping Context to be used for player input */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputMappingContext* InputMappingContext;

	virtual void BeginPlay() override;

	virtual void AcknowledgePossession(APawn* P) override;

	// End Actor interface
};

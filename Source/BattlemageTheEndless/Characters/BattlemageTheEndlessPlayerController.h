// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "BattlemageTheEndless/Abilities/GA_WithEffectsBase.h"
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
	
	UFUNCTION(Server, Reliable)
	void Server_ApplyEffects(TSubclassOf<UGA_WithEffectsBase> EffectClass, const FHitResult& Hit);
	
protected:

	/** Input Mapping Context to be used for player input */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputMappingContext* InputMappingContext;

	virtual void BeginPlay() override;

	virtual void AcknowledgePossession(APawn* P) override;

	// End Actor interface
};

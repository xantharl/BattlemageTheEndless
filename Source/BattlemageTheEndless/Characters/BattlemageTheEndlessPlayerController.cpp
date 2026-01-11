// Copyright Epic Games, Inc. All Rights Reserved.


#include "BattlemageTheEndlessPlayerController.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "BattlemageTheEndlessCharacter.h"
#include "EnhancedInputSubsystems.h"

void ABattlemageTheEndlessPlayerController::Server_HandleMovementEvent_Implementation(const FGameplayTag EventTag, const FMovementEventData& MovementEventData)
{
	if (!AcknowledgedPawn)
	{
		UE_LOG(LogTemp, Warning, TEXT("Server_HandleMovementEvent called but AcknowledgedPawn is null"));
		return;
	}
	if (auto Movement = Cast<UBMageCharacterMovementComponent>(AcknowledgedPawn->GetMovementComponent()))
	{
		auto dodge = Cast<UDodgeAbility>(Movement->MovementAbilities[MovementAbilityType::Dodge]);
		dodge->Begin(MovementEventData);
	}
}

void ABattlemageTheEndlessPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// get the enhanced input subsystem
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		// add the mapping context so we get controls
		Subsystem->AddMappingContext(InputMappingContext, 0);

		UE_LOG(LogTemp, Warning, TEXT("BeginPlay"));
	}
}

void ABattlemageTheEndlessPlayerController::AcknowledgePossession(APawn* P)
{
	Super::AcknowledgePossession(P);

	// ABattlemageTheEndlessCharacter* CharacterBase = Cast<ABattlemageTheEndlessCharacter>(P);
	// if (CharacterBase)
	// {
	// 	CharacterBase->AbilitySystemComponent->InitAbilityActorInfo(CharacterBase, CharacterBase);
	// }
}

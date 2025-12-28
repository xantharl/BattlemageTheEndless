// Copyright Epic Games, Inc. All Rights Reserved.


#include "BattlemageTheEndlessPlayerController.h"
#include "BattlemageTheEndlessCharacter.h"
#include "EnhancedInputSubsystems.h"

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
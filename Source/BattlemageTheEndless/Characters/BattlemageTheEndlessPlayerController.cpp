// Copyright Epic Games, Inc. All Rights Reserved.


#include "BattlemageTheEndlessPlayerController.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "BattlemageTheEndlessCharacter.h"
#include "EnhancedInputSubsystems.h"
#include "BattlemageTheEndless/Abilities/EventData/DodgeEventData.h"

void ABattlemageTheEndlessPlayerController::Server_HandleMovementEvent_Implementation(FGameplayTag EventTag, FVector OptionalVector)
{
	if (!AcknowledgedPawn)
	{
		UE_LOG(LogTemp, Warning, TEXT("Server_HandleMovementEvent called but AcknowledgedPawn is null"));
		return;
	}
	if (UAbilitySystemComponent* ASC = AcknowledgedPawn->FindComponentByClass<UAbilitySystemComponent>())
	{
		auto Optional = NewObject<UDodgeEventData>(AcknowledgedPawn);
		Optional->DodgeInputVector = OptionalVector;
		FGameplayEventData EventData;
		EventData.Instigator = AcknowledgedPawn;
		EventData.Target = AcknowledgedPawn;
		EventData.EventTag = EventTag;
		EventData.OptionalObject = Optional;
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(this, EventData.EventTag,EventData);
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

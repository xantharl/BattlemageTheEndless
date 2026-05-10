// Copyright Epic Games, Inc. All Rights Reserved.


#include "BattlemageTheEndlessPlayerController.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "BattlemageTheEndlessCharacter.h"
#include "EnhancedInputSubsystems.h"
#include "BattlemageTheEndless/BMageAbilitySystemComponent.h"

void ABattlemageTheEndlessPlayerController::Server_HandleMovementEvent_Implementation(const FGameplayTag EventTag, const FMovementEventData& MovementEventData)
{
	if (!AcknowledgedPawn)
	{
		UE_LOG(LogTemp, Warning, TEXT("Server_HandleMovementEvent called but AcknowledgedPawn is null"));
		return;
	}
	const auto Movement = Cast<UBMageCharacterMovementComponent>(AcknowledgedPawn->GetMovementComponent());
	if (!Movement)
	{
		UE_LOG( LogTemp, Warning, TEXT("Server_HandleMovementEvent called but AcknowledgedPawn has no UBMageCharacterMovementComponent"));
		return;
	}
	// Work backwards from the Tag to determine which ability we want to activate
	const MovementAbilityType AbilityType = UMovementAbility::GetMovementAbilityTypeFromTag(EventTag);
	if (AbilityType == MovementAbilityType::None)
	{
		UE_LOG(LogTemp, Warning, TEXT("Server_HandleMovementEvent called with invalid ability tag: %s"), *EventTag.ToString());
		return;
	}		
	
	if (const auto Ability = Movement->MovementAbilities.Find(AbilityType))
		(*Ability)->Begin(MovementEventData);
}

void ABattlemageTheEndlessPlayerController::Server_ResolveHit_Implementation(TSubclassOf<UGA_WithEffectsBase> AbilityClass, const FHitResult& Hit)
{
	if (!AcknowledgedPawn)
	{
		UE_LOG(LogTemp, Warning, TEXT("Server_ResolveHit called but AcknowledgedPawn is null"));
		return;
	}
	auto AttackerCharacter = Cast<ACharacter>(AcknowledgedPawn);
	if (!AttackerCharacter) return;

	auto AttackerASC = Cast<UBMageAbilitySystemComponent>(AcknowledgedPawn->FindComponentByClass<UAbilitySystemComponent>());
	if (!AttackerASC) return;

	auto Spec = AttackerASC->FindAbilitySpecFromClass(AbilityClass);
	if (!Spec)
	{
		UE_LOG(LogTemp, Warning, TEXT("Server_ResolveHit: no ability spec found for class %s"), *AbilityClass->GetName());
		return;
	}
	auto Ability = Cast<UGA_WithEffectsBase>(Spec->Ability);
	if (!Ability) return;

	AttackerASC->ResolveAuthoritativeHit(AttackerCharacter, Hit, Ability, Ability->GetAssetTags());
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

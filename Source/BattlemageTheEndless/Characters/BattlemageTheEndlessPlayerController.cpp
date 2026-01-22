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

void ABattlemageTheEndlessPlayerController::Server_ApplyEffects_Implementation(TSubclassOf<UGA_WithEffectsBase> EffectClass, const FHitResult& Hit)
{
	if (!AcknowledgedPawn)
	{
		UE_LOG(LogTemp, Warning, TEXT("Server_ApplyEffects_Implementation called but AcknowledgedPawn is null"));
		return;
	}
	const auto ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(AcknowledgedPawn);
	if (!ASC)
	{
		UE_LOG( LogTemp, Warning, TEXT("Server_HandleMovementEvent called but AcknowledgedPawn has no ASC"));
		return;
	}	
	const auto HitActorASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Hit.GetActor());
	if (!HitActorASC)
	{
		UE_LOG( LogTemp, Warning, TEXT("Server_HandleMovementEvent called but HitActor has no ASC"));
		return;
	}
	
	auto Spec = ASC->FindAbilitySpecFromClass(EffectClass);
	if (!Spec)
	{
		UE_LOG(LogTemp, Warning, TEXT("Server_ApplyEffects_Implementation called but no ability spec found for class %s"), *EffectClass->GetName());
		return;
	}
	auto Ability = Cast<UGA_WithEffectsBase>(Spec->Ability);
	if (!Ability)
	{
		UE_LOG(LogTemp, Warning, TEXT("Server_ApplyEffects_Implementation called but ability %s is not of type UGA_WithEffectsBase"), *Ability->GetName());
		return;
	}
	
	Ability->ApplyEffects(Hit.GetActor(), HitActorASC, AcknowledgedPawn, GetOwner());
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

// Copyright Epic Games, Inc. All Rights Reserved.

#include "TP_WeaponComponent.h"
#include "BattlemageTheEndlessProjectile.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "Kismet/GameplayStatics.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "../Characters/BattlemageTheEndlessCharacter.h"

// Sets default values for this component's properties
UTP_WeaponComponent::UTP_WeaponComponent()
{
	// Default offset from the character location for projectiles to spawn
	MuzzleOffset = FVector(10.0f, 0.0f, 10.0f);
}

void UTP_WeaponComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (Character == nullptr)
	{
		return;
	}

	RemoveContext();
}

void UTP_WeaponComponent::RemoveContext()
{
	if (APlayerController* PlayerController = Cast<APlayerController>(Character->GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->RemoveMappingContext(WeaponMappingContext);
		}
	}
}

void UTP_WeaponComponent::AddBindings()
{
	// TODO: Unbreak this
	//if (APlayerController* PlayerController = Cast<APlayerController>(Character->GetController()))
	//{
	//	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerController->InputComponent))
	//	{
	//		// TODO: Figure out how to make this dynamic and allow BP set of these params, struct?
	//		EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Triggered, this, &UTP_WeaponComponent::Fire);
	//		EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Completed, this, &UTP_WeaponComponent::SuspendAttackSequence);
	//	}
	//}
}

// This is called by the AnimNotify_Collision Blueprint
void UTP_WeaponComponent::OnAnimTraceHit(const FHitResult& Hit)
{
	// If we hit a Battlemage character, apply damage	
	if (!Hit.HitObjectHandle.DoesRepresentClass(ABattlemageTheEndlessCharacter::StaticClass()))
		return;
	
	ABattlemageTheEndlessCharacter* otherCharacter = Cast<ABattlemageTheEndlessCharacter>(Hit.HitObjectHandle.FetchActor());

	// If this character has already been hit by this stage of the combo, don't hit them again
	auto characterIter = LastHitCharacters.find(Cast<ABattlemageTheEndlessCharacter>(otherCharacter));
	if (characterIter != LastHitCharacters.end() && characterIter->second == ComboAttackNumber)
	{
		return;
	}

	// Stop hitting yourself
	if (otherCharacter == Character)
	{
		return;
	}

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 1.50f, FColor::Yellow, FString::Printf(TEXT("Hit on ComboAttackNumber %i"), ComboAttackNumber));
	}

	LastHitCharacters.insert_or_assign(otherCharacter, ComboAttackNumber);
	float Damage = LightAttackDamage;
	otherCharacter->ApplyDamage(Damage);
}
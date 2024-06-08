// Copyright Epic Games, Inc. All Rights Reserved.

#include "TP_WeaponComponent.h"
#include "BattlemageTheEndlessCharacter.h"
#include "BattlemageTheEndlessProjectile.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "Kismet/GameplayStatics.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"

// Sets default values for this component's properties
UTP_WeaponComponent::UTP_WeaponComponent()
{
	// Default offset from the character location for projectiles to spawn
	MuzzleOffset = FVector(100.0f, 0.0f, 10.0f);
}


void UTP_WeaponComponent::Fire()
{
	if (Character == nullptr || Character->GetController() == nullptr)
	{
		return;
	}

	// Try and fire a projectile
	if (ProjectileClass != nullptr)
	{
		UWorld* const World = GetWorld();
		if (World != nullptr)
		{
			APlayerController* PlayerController = Cast<APlayerController>(Character->GetController());
			const FRotator SpawnRotation = PlayerController->PlayerCameraManager->GetCameraRotation();
			// MuzzleOffset is in camera space, so transform it to world space before offsetting from the character location to find the final muzzle position
			const FVector SpawnLocation = GetOwner()->GetActorLocation() + SpawnRotation.RotateVector(MuzzleOffset);
	
			//Set Spawn Collision Handling Override
			FActorSpawnParameters ActorSpawnParams;
			ActorSpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding;
	
			// Spawn the projectile at the muzzle
			World->SpawnActor<ABattlemageTheEndlessProjectile>(ProjectileClass, SpawnLocation, SpawnRotation, ActorSpawnParams);
		}
	}
	
	// Try and play the sound if specified
	if (FireSound != nullptr)
	{
		UGameplayStatics::PlaySoundAtLocation(this, FireSound, Character->GetActorLocation());
	}
	
	// Try and play a firing animation if specified
	if (FireAnimation != nullptr)
	{
		// Get the animation object for the arms mesh
		UAnimInstance* AnimInstance = Character->GetMesh()->GetAnimInstance();
		if (AnimInstance != nullptr)
		{
			// If we are mid attack montage...
			if (AnimInstance->GetCurrentActiveMontage() && AnimInstance->GetCurrentActiveMontage()->GetName() == FireAnimation->GetName())
			{
				// Resume if paused
				if (!AnimInstance->Montage_IsPlaying(FireAnimation))
				{
					AnimInstance->Montage_Resume(FireAnimation);
					AttackRequested = false;
				}

				// Otherwise register that the next attack has been requested
				// TODO: Make this request auto-expire for the benefit of long duration attacks
				else
					AttackRequested = true;
			}
			// If we aren't playing a montage start one
			else 
				AnimInstance->Montage_Play(FireAnimation, 1.f);
		}
	}
}

void UTP_WeaponComponent::AttachWeapon(ABattlemageTheEndlessCharacter* TargetCharacter)
{
	Character = TargetCharacter;

	// Check that the character is valid, and has no rifle yet
	if (Character == nullptr || Character->GetHasWeapon())
	{
		return;
	}

	// Attach the weapon to the First Person Character
	FAttachmentTransformRules AttachmentRules(EAttachmentRule::SnapToTarget, true);
	AttachToComponent(Character->GetMesh(), AttachmentRules, FName(TEXT("GripPoint")));
	
	// switch bHasRifle so the animation blueprint can switch to another animation set
	Character->SetHasWeapon(this);

	// Set up action bindings
	if (APlayerController* PlayerController = Cast<APlayerController>(Character->GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			// Set the priority of the mapping to 1, so that it overrides the Jump action with the Fire action when using touch input
			Subsystem->AddMappingContext(FireMappingContext, 1);
		}

		if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerController->InputComponent))
		{
			// Fire
			EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Triggered, this, &UTP_WeaponComponent::Fire);
			EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Completed, this, &UTP_WeaponComponent::SuspendAttackSequence);
		}
	}
}

void UTP_WeaponComponent::DetachWeapon()
{
	// Invalid state, we should only be dropping attached weapons
	if (Character == nullptr || !Character->GetHasWeapon())
	{
		return;
		// TODO: Figure out how to log an error
	}
	DetachFromComponent(FDetachmentTransformRules(EDetachmentRule::KeepRelative, false));
	Character->SetHasWeapon(nullptr);
	RemoveContext();
	if (APlayerController* PlayerController = Cast<APlayerController>(Character->GetController()))
	{
		if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerController->InputComponent))
		{
			int removed = 0;

			int bindingCount = EnhancedInputComponent->GetNumActionBindings();
			for (int i = bindingCount - 1; i >= 0; --i)
			{
				if (FireAction->GetName() == (EnhancedInputComponent->GetActionBinding(i)).GetActionName())
				{
					EnhancedInputComponent->RemoveActionEventBinding(i);
					removed += 1;
					// If we've removed both the Triggered and Complete bindings, we're done
					if (removed == 2)
						break;
				}
			}
		}
	}

	this->SetRelativeLocation(Character->GetActorLocation());
	WeaponDropped.Broadcast();
}

void UTP_WeaponComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (Character == nullptr)
	{
		return;
	}

	RemoveContext();
}

void UTP_WeaponComponent::SuspendAttackSequence()
{
	// Make sure we won't crash the program
	if (FireAnimation != nullptr)
	{
		// Get the animation object for the mesh
		UAnimInstance* AnimInstance = Character->GetMesh()->GetAnimInstance();
		if (AnimInstance != nullptr)
		{
			// Get current position, I *think* this is time rather than frame
			float position = AnimInstance->GetActiveMontageInstance()->GetPosition();

			FAnimNotifyContext notifyContext = FAnimNotifyContext();
			FireAnimation->GetAnimNotifies(position, 1.f, notifyContext);

			// If there are no notifies, this is the last attack, just let it finish
			int notifyCount = notifyContext.ActiveNotifies.Num();
			if (notifyCount == 0)
				return;

			// Find the first UAnimNotify_PlayMontageNotify, dispatch the timer based callback, and return
			for (int i = 0; i < notifyCount; ++i) 
			{
				const FAnimNotifyEvent* notify = notifyContext.ActiveNotifies[i].GetNotify();
				if (notify->NotifyName == "PlayMontageNotify")
				{
					float waitBeforePause = notify->GetTriggerTime() - position;
					Character->GetWorldTimerManager().SetTimer(WaitForPauseTime, this, &UTP_WeaponComponent::PauseOrContinueCombo, waitBeforePause, false);
					return;
				}
			}
		}
	}
}

void UTP_WeaponComponent::PauseOrContinueCombo()
{
	// Make sure we won't crash the program
	if (FireAnimation != nullptr)
	{
		// Get the animation object for the mesh
		UAnimInstance* AnimInstance = Character->GetMesh()->GetAnimInstance();
		if (AnimInstance != nullptr)
		{
			if (AttackRequested)
			{
				AttackRequested = false;
				return;
			}
			AnimInstance->Montage_Pause(FireAnimation);
			Character->GetWorldTimerManager().SetTimer(TimeSinceComboPaused, this, &UTP_WeaponComponent::EndComboIfStillPaused, ComboBreakTime, false);
		}
	}
}

void UTP_WeaponComponent::EndComboIfStillPaused()
{
	// Make sure we won't crash the program
	if (FireAnimation != nullptr)
	{
		// Get the animation object for the mesh
		UAnimInstance* AnimInstance = Character->GetMesh()->GetAnimInstance();
		if (AnimInstance != nullptr && !AnimInstance->Montage_IsPlaying(FireAnimation))
		{
				AnimInstance->Montage_Stop(0.25, FireAnimation);
		}
	}
}

void UTP_WeaponComponent::RemoveContext()
{
	if (APlayerController* PlayerController = Cast<APlayerController>(Character->GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->RemoveMappingContext(FireMappingContext);
		}
	}
}
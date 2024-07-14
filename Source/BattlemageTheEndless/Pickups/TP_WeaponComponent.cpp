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


void UTP_WeaponComponent::Fire()
{
	if (Character == nullptr || Character->GetController() == nullptr)
	{
		return;
	}

	// Try and fire a projectile, limit by rate of fire
	time_t secondsBetweenShots = 60.f / ShotsPerMinute;
	if (ProjectileClass != nullptr && LastFireTime < time(0) - secondsBetweenShots)
	{
		UWorld* const World = GetWorld();
		if (World != nullptr)
		{
			APlayerController* PlayerController = Cast<APlayerController>(Character->GetController());
			const FRotator SpawnRotation = PlayerController->PlayerCameraManager->GetCameraRotation();
			// MuzzleOffset is in camera space, so transform it to world space before offsetting from the character location to find the final muzzle position
			FName socketName = Character->GetLeftHandWeapon() == this ? FName("GripLeft") : FName("GripRight");
			// Spawn the projectile at the attachment point of the weapon with respect for offsets
			const FVector SpawnLocation = Character->GetMesh()->GetSocketLocation(socketName) + SpawnRotation.RotateVector(MuzzleOffset) + SpawnRotation.RotateVector(AttachmentOffset);
	
			//Set Spawn Collision Handling Override
			FActorSpawnParameters ActorSpawnParams;
			ActorSpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding;
	
			// Spawn the projectile at the muzzle
			World->SpawnActor<ABattlemageTheEndlessProjectile>(ProjectileClass, SpawnLocation, SpawnRotation, ActorSpawnParams);

			LastFireTime = time(0);
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
					ComboAttackNumber += 1;
				}

				// Otherwise register that the next attack has been requested
				// TODO: Make this request auto-expire for the benefit of long duration attacks
				else
					AttackRequested = true;
			}
			// If we aren't playing a montage start one
			else 
			{
				// In a perfect world this isn't needed, but in this world it's a race condition
				ComboAttackNumber = 0;
				AnimInstance->Montage_Play(FireAnimation, 1.f);
			}
		}
	}
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
			// Race condition, exit if we finished the animation
			if (!AnimInstance->GetActiveMontageInstance())
				return;

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
				ComboAttackNumber += 1;
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
				ComboAttackNumber = 0;
				LastHitCharacters.clear();
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

void UTP_WeaponComponent::AddBindings()
{
	if (APlayerController* PlayerController = Cast<APlayerController>(Character->GetController()))
	{
		if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerController->InputComponent))
		{
			// Fire
			EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Triggered, this, &UTP_WeaponComponent::Fire);
			EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Completed, this, &UTP_WeaponComponent::SuspendAttackSequence);
		}
	}
}

// This is called by the AnimNotify_Collision Blueprint
void UTP_WeaponComponent::OnAnimTraceHit(const FHitResult& Hit)
{
	// If we hit a Battlemage character, apply damage	
	if (Hit.HitObjectHandle.DoesRepresentClass(ABattlemageTheEndlessCharacter::StaticClass()))
	{
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
}
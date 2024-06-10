// Copyright Epic Games, Inc. All Rights Reserved.

#include "BattlemageTheEndlessCharacter.h"
#include "BattlemageTheEndlessProjectile.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PawnMovementComponent.h"
#include <GameFramework/FloatingPawnMovement.h>
#include <format>
#include <Kismet/GameplayStatics.h>
#include "ABMageCharacterMovementComponent.h"
DEFINE_LOG_CATEGORY(LogTemplateCharacter);

//////////////////////////////////////////////////////////////////////////
// ABattlemageTheEndlessCharacter


ABattlemageTheEndlessCharacter::ABattlemageTheEndlessCharacter()
{
	// Character doesnt have a rifle at start
	bHasRifle = false;
	bIsSprinting = false;
	bIsSliding = false;
	bShouldUncrouch = false;
	slideElapsedSeconds = 0.0f;
	MaxLaunches = 1;
	launchesPerformed = 0;

	// Create FirstPersonCamera
	FirstPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCamera->SetupAttachment(GetMesh());
	FirstPersonCamera->SetRelativeLocation(FVector(0.f, 8.8f, 140.f)); // Position the camera
	FirstPersonCamera->SetRelativeRotation(FRotator(0.f, 90.f, 0.f)); // Position the camera
	FirstPersonCamera->bUsePawnControlRotation = true;

	// Camera Boom
	ThirdPersonCameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("ThirdPersonCameraBoom"));
	ThirdPersonCameraBoom->SetupAttachment(GetCapsuleComponent());
	ThirdPersonCameraBoom->SetRelativeLocation(FVector(0.f, 0.f, 8.5f)); // Position the camera
	ThirdPersonCameraBoom->ProbeSize = 12.f;
	ThirdPersonCameraBoom->ProbeChannel = ECollisionChannel::ECC_Camera;
	ThirdPersonCameraBoom->TargetArmLength = 400.f;

	// Create a CameraComponent	
	ThirdPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("ThirdPersonCamera"));
	ThirdPersonCamera->SetupAttachment(ThirdPersonCameraBoom);
	ThirdPersonCamera->SetRelativeLocation(FVector(210.f, 0.f, 98.f)); // Position the camera
	ThirdPersonCamera->SetRelativeRotation(FRotator(-22.f, 0.f, 0.f)); // Position the camera
	ThirdPersonCamera->bUsePawnControlRotation = true;

	// Init Audio Resource
	static ConstructorHelpers::FObjectFinder<USoundWave> Sound(TEXT("/Game/Sounds/Jump_Landing"));
	JumpLandingSound = Sound.Object;
	
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(55.f, 96.0f);

	JumpMaxCount = 2;

	TObjectPtr<UCharacterMovementComponent> movement = GetCharacterMovement();
	if (movement)
	{
		movement->AirControl = 0.8f;
		movement->GetNavAgentPropertiesRef().bCanCrouch = true;
	}

}

void ABattlemageTheEndlessCharacter::BeginPlay()
{
	// Call the base class  
	Super::BeginPlay();

	// Add Input Mapping Context
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}

}

//////////////////////////////////////////////////////////////////////////// Input

void ABattlemageTheEndlessCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		// Launch Jump
		EnhancedInputComponent->BindAction(LaunchJumpAction, ETriggerEvent::Started, this, &ABattlemageTheEndlessCharacter::LaunchJump);
		EnhancedInputComponent->BindAction(LaunchJumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		// Crouching
		EnhancedInputComponent->BindAction(CrouchAction, ETriggerEvent::Started, this, &ABattlemageTheEndlessCharacter::ToggleCrouch);
		EnhancedInputComponent->BindAction(CrouchAction, ETriggerEvent::Completed, this, &ABattlemageTheEndlessCharacter::TryUnCrouch);

		// Sprinting
		EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Started, this, &ABattlemageTheEndlessCharacter::ToggleSprint);
		EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Completed, this, &ABattlemageTheEndlessCharacter::ToggleSprint);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ABattlemageTheEndlessCharacter::Move);

		// Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ABattlemageTheEndlessCharacter::Look);

		// Switch Camera
		EnhancedInputComponent->BindAction(SwitchCameraAction, ETriggerEvent::Triggered, this, &ABattlemageTheEndlessCharacter::SwitchCamera);
	}
	else
	{
		UE_LOG(LogTemplateCharacter, Error, TEXT("'%s' Failed to find an Enhanced Input Component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}

void ABattlemageTheEndlessCharacter::ToggleCrouch() 
{
	if (bIsCrouched)
	{
		ACharacter::UnCrouch();
	}
	else 
	{
		ACharacter::Crouch(false);

		UCharacterMovementComponent* movement = GetCharacterMovement();
		if (bIsSprinting && movement)
		{
			bIsSliding = true;
			// default walk is 1200, crouch is 300
			movement->MaxWalkSpeedCrouched = SprintSpeed;
		}
	}
}

void ABattlemageTheEndlessCharacter::TryUnCrouch()
{
	if (!bIsCrouched)
		return;

	if (bIsSliding)
	{
		bShouldUncrouch = true;
		return;
	}

	UnCrouch();
}

void ABattlemageTheEndlessCharacter::TickActor(float DeltaTime, ELevelTick TickType, FActorTickFunction& ThisTickFunction)
{
	// If we're sliding, handle it, this is mutually exclusing with the uncrouch case
	if (bIsSliding)
	{
		UCharacterMovementComponent* movement = GetCharacterMovement();
		slideElapsedSeconds += DeltaTime;
		if (movement)
		{
			// If we've reached the slide's full duration, reset speed and slide data
			if (slideElapsedSeconds >= SlideDurationSeconds)
			{
				EndSlide(movement);
			}
			else // Otherwise decrement the slide speed
			{
				float newSpeed = SprintSpeed * powf(2.7182818284f, -1.39f * slideElapsedSeconds);
				movement->MaxWalkSpeedCrouched = newSpeed;
				/*if (GEngine)
				{
					GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, FString::Printf(TEXT("Elapsed: %f, Setting crouch speed to %f"), slideElapsedSeconds, newSpeed));
				}*/
			}
		}
	}
	// If we have a pending uncrouch, uncrouch. This is for the post slide case
	else if (bShouldUncrouch)
	{
		UnCrouch();
		bShouldUncrouch = false;
	}

	AActor::TickActor(DeltaTime, TickType, ThisTickFunction);
}

void ABattlemageTheEndlessCharacter::EndSlide(UCharacterMovementComponent* movement)
{
	movement->MaxWalkSpeedCrouched = CrouchSpeed;
	bIsSliding = false;
	slideElapsedSeconds = 0.0f;
}

void ABattlemageTheEndlessCharacter::ToggleSprint()
{
	TObjectPtr<UCharacterMovementComponent> movement = GetCharacterMovement();
	if (!bIsSprinting && movement)
	{
		// if sliding, end it before sprinting
		if (bIsSliding)
		{
			EndSlide(movement);
			UnCrouch();
		}

		movement->MaxWalkSpeed = SprintSpeed;
		bIsSprinting = true;
	}
	else if(movement)
	{
		movement->MaxWalkSpeed = WalkSpeed;
		bIsSprinting = false;
	}
}

void ABattlemageTheEndlessCharacter::LaunchJump()
{
	if (launchesPerformed >= MaxLaunches)
	{
		// If we can't launch anymore, redirect to jump logic (which checks jump count and handles appropriately)
		Jump();
		return;
	}

	launchesPerformed += 1;

	TObjectPtr<UCharacterMovementComponent> movement = GetCharacterMovement();
	FRotator rotator = GetActorRotation();
	/*if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, FString::Printf(TEXT("Launch Jump Starting Vector %s"), *GetActorForwardVector().ToString()));
	}*/
	rotator.Pitch = 0;
	FVector vector = *new FVector(LaunchSpeedHorizontal, 0.0f, LaunchSpeedVertical);
	vector = vector.RotateAngleAxis(rotator.Yaw, FVector::ZAxisVector);
	/*if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, FString::Printf(TEXT("Launching with vector %s"), *vector.ToString()));
	}*/
	LaunchCharacter(vector, false, true);
	/*if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, FString::Printf(TEXT("Launch Jump Ending Vector %s"), *GetActorForwardVector().ToString()));
	}*/
	JumpCurrentCount += 1;
}


void ABattlemageTheEndlessCharacter::Move(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// add movement 
		AddMovementInput(GetActorForwardVector(), MovementVector.Y);
		AddMovementInput(GetActorRightVector(), MovementVector.X);
	}
}

void ABattlemageTheEndlessCharacter::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}

void ABattlemageTheEndlessCharacter::SetHasWeapon(UTP_WeaponComponent* weapon)
{
	bHasRifle = weapon != nullptr;
	HeldWeapon = weapon;
}

bool ABattlemageTheEndlessCharacter::GetHasWeapon()
{
	return bHasRifle;
}

UTP_WeaponComponent* ABattlemageTheEndlessCharacter::GetHeldWeapon()
{
	return HeldWeapon;
}

USkeletalMeshComponent* ABattlemageTheEndlessCharacter::GetHeldWeaponMeshComponent()
{
	return (USkeletalMeshComponent*)HeldWeapon;
}

void ABattlemageTheEndlessCharacter::OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode)
{
	if (PrevMovementMode == EMovementMode::MOVE_Falling)
	{
		launchesPerformed = 0;
		UGameplayStatics::PlaySoundAtLocation(this,
			JumpLandingSound,
			GetActorLocation(), 1.0f);
	}

	ACharacter::OnMovementModeChanged(PrevMovementMode, PreviousCustomMode);
}

void ABattlemageTheEndlessCharacter::SwitchCamera() 
{
	time_t now = time(0);
	if (now - _lastCameraSwap < 0.5f)
		return;

	_lastCameraSwap = time(0);
	if (ThirdPersonCamera->IsActive())
	{
		ThirdPersonCamera->Deactivate();
		FirstPersonCamera->Activate();
	}
	else 
	{
		ThirdPersonCamera->Activate();
		FirstPersonCamera->Deactivate();
	}

	Cast<APlayerController>(GetController())->SetViewTargetWithBlend((AActor*)this);
}

void ABattlemageTheEndlessCharacter::ApplyDamage(float damage)
{
	Health -= damage > Health ? Health : damage;
}
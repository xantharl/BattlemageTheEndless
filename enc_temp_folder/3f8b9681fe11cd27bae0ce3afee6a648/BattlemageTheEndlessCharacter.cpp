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
DEFINE_LOG_CATEGORY(LogTemplateCharacter);

//////////////////////////////////////////////////////////////////////////
// ABattlemageTheEndlessCharacter


ABattlemageTheEndlessCharacter::ABattlemageTheEndlessCharacter()
{
	// Character doesnt have a rifle at start
	bIsSprinting = false;
	IsSliding = false;
	bShouldUncrouch = false;
	slideElapsedSeconds = 0.0f;
	launchesPerformed = 0;

	SetupCameras();

	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(55.f, 96.0f);

	// Init Audio Resource
	static ConstructorHelpers::FObjectFinder<USoundWave> Sound(TEXT("/Game/Sounds/Jump_Landing"));
	JumpLandingSound = Sound.Object;


	JumpMaxCount = 2;

	TObjectPtr<UCharacterMovementComponent> movement = GetCharacterMovement();
	if (movement)
	{
		movement->AirControl = 0.8f;
		movement->GetNavAgentPropertiesRef().bCanCrouch = true;
	}

}

void ABattlemageTheEndlessCharacter::SetupCameras()
{
	// Create FirstPersonCamera
	FirstPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	// TODO: This throws an exception on startup, doesn't actually break anything but annoying
	FirstPersonCamera->AttachToComponent(GetMesh(), FAttachmentTransformRules::KeepRelativeTransform, FName("cameraSocket"));
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
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ABattlemageTheEndlessCharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		// Launch Jump
		EnhancedInputComponent->BindAction(LaunchJumpAction, ETriggerEvent::Started, this, &ABattlemageTheEndlessCharacter::LaunchJump);
		EnhancedInputComponent->BindAction(LaunchJumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		// Crouching
		EnhancedInputComponent->BindAction(CrouchAction, ETriggerEvent::Started, this, &ABattlemageTheEndlessCharacter::Crouch);
		EnhancedInputComponent->BindAction(CrouchAction, ETriggerEvent::Completed, this, &ABattlemageTheEndlessCharacter::RequestUnCrouch);

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

void ABattlemageTheEndlessCharacter::Crouch() 
{
	// nothing to do if we're already crouching
	if (bIsCrouched)
		return;

	ACharacter::Crouch(false);

	if (!bIsSprinting)
	{
		return;
	}

	UCharacterMovementComponent* movement = GetCharacterMovement();
	if (& movement)
	{
		IsSliding = true;
		movement->SetCrouchedHalfHeight(SlideHalfHeight);
		// default walk is 1200, crouch is 300
		movement->MaxWalkSpeedCrouched = SprintSpeed;
	}
}

void ABattlemageTheEndlessCharacter::RequestUnCrouch()
{
	if (!bIsCrouched)
		return;

	bShouldUncrouch = true;
}

void ABattlemageTheEndlessCharacter::TickActor(float DeltaTime, ELevelTick TickType, FActorTickFunction& ThisTickFunction)
{
	// If we're sliding, handle it, this is mutually exclusing with the uncrouch case
	if (IsSliding)
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
				float newSpeed = SlideSpeed * powf(2.7182818284f, -1.39f * slideElapsedSeconds);
				movement->MaxWalkSpeedCrouched = newSpeed;
			}
		}
	}

	// If we have a pending uncrouch, uncrouch. Evaluate whether a launch triggered it
	if (bShouldUncrouch)
	{
		UnCrouch(false);
		bShouldUncrouch = false;

		if (IsSliding)
		{
			EndSlide(GetCharacterMovement());
		}
		if (bLaunchRequested)
		{
			UCharacterMovementComponent* movement = GetCharacterMovement();
			DoLaunchJump();
			bLaunchRequested = false;
		}
	}

	AActor::TickActor(DeltaTime, TickType, ThisTickFunction);
}

void ABattlemageTheEndlessCharacter::EndSlide(UCharacterMovementComponent* movement)
{
	movement->MaxWalkSpeedCrouched = bIsCrouched ? CrouchSpeed : WalkSpeed;
	IsSliding = false;
	slideElapsedSeconds = 0.0f;
	movement->SetCrouchedHalfHeight(CrouchedHalfHeight);
}

void ABattlemageTheEndlessCharacter::ToggleSprint()
{
	TObjectPtr<UCharacterMovementComponent> movement = GetCharacterMovement();
	if (!bIsSprinting && movement)
	{
		// if sliding, end it before sprinting
		if (IsSliding)
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
	// TODO: Remove maxLaunches property, don't think I want more than 1 ever
	if (launchesPerformed >= MaxLaunches)
	{
		// If we can't launch anymore, redirect to jump logic (which checks jump count and handles appropriately)
		Jump();
		return;
	}

	bLaunchRequested = true;
	RequestUnCrouch();
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

		TObjectPtr<UCharacterMovementComponent> movement = GetCharacterMovement();
		if (!movement)
			return;

		FVector forwardVector = movement->Velocity;
		forwardVector = FVector(forwardVector.RotateAngleAxis(movement->GetLastUpdateRotation().GetInverse().Yaw, FVector::ZAxisVector));

		// if we've started moving backwards, set the speed to ReverseSpeed
 		if (forwardVector.X < 0 && movement->MaxWalkSpeed != ReverseSpeed)
		{
			movement->MaxWalkSpeed = ReverseSpeed;
		}
		// otherwise set it to WalkSpeed
		else if (forwardVector.X >= 0 && movement->MaxWalkSpeed == ReverseSpeed)
		{
			movement->MaxWalkSpeed = WalkSpeed;
		}
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

void ABattlemageTheEndlessCharacter::Jump()
{
	if (IsSliding)
	{
		EndSlide(GetCharacterMovement());
	}

	if (bIsCrouched)
	{
		UnCrouch();
	}

	if (JumpCurrentCount < JumpMaxCount)
	{
		Super::Jump();

		UCharacterMovementComponent* movement = GetCharacterMovement();
		if (movement && movement->MovementMode == EMovementMode::MOVE_Falling)
		{
			// redirect the character's current velocity in the direction they're facing
			float yawDifference = GetBaseAimRotation().Yaw - movement->Velocity.Rotation().Yaw;
			movement->Velocity = movement->Velocity.RotateAngleAxis(yawDifference, FVector::ZAxisVector);
		}
	}
}

void ABattlemageTheEndlessCharacter::SetLeftHandWeapon(UTP_WeaponComponent* weapon)
{
	LeftHandWeapon = weapon;
}

void ABattlemageTheEndlessCharacter::SetRightHandWeapon(UTP_WeaponComponent* weapon)
{
	RightHandWeapon = weapon;
}

bool ABattlemageTheEndlessCharacter::GetHasLeftHandWeapon()
{
	return LeftHandWeapon != NULL;
}

bool ABattlemageTheEndlessCharacter::GetHasRightHandWeapon()
{
	return RightHandWeapon != NULL;
}

UTP_WeaponComponent* ABattlemageTheEndlessCharacter::GetWeapon(EquipSlot SlotType)
{
	if (LeftHanded)
	{
		return SlotType == EquipSlot::Primary ? LeftHandWeapon : RightHandWeapon;
	}
	else
	{
		return SlotType == EquipSlot::Primary ? RightHandWeapon : LeftHandWeapon;
	}
	
}

UTP_WeaponComponent* ABattlemageTheEndlessCharacter::GetLeftHandWeapon()
{
	return LeftHandWeapon;
}

UTP_WeaponComponent* ABattlemageTheEndlessCharacter::GetRightHandWeapon()
{
	return RightHandWeapon;
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

void ABattlemageTheEndlessCharacter::DoLaunchJump()
{
	launchesPerformed += 1;

	// Create the launch vector
	TObjectPtr<UCharacterMovementComponent> movement = GetCharacterMovement();
	FRotator rotator = GetActorRotation();
	rotator.Pitch = 0;
	FVector vector = *new FVector(LaunchSpeedHorizontal, 0.0f, LaunchSpeedVertical);
	vector = vector.RotateAngleAxis(rotator.Yaw, FVector::ZAxisVector);

	// Launch the character
	//EndSlide(movement);
	LaunchCharacter(vector, false, true);
}

void ABattlemageTheEndlessCharacter::ApplyDamage(float damage)
{
	Health -= damage > Health ? Health : damage;
}

FName ABattlemageTheEndlessCharacter::GetTargetSocketName(EquipSlot SlotType)
{
	if (SlotType == EquipSlot::Primary)
	{
		return FName(LeftHanded ? "GripLeft" : "GripRight");
	}
	else
	{
		return FName(LeftHanded ? "GripRight" : "GripLeft");
	}
}

bool ABattlemageTheEndlessCharacter::TrySetWeapon(UTP_WeaponComponent* Weapon, FName SocketName)
{
	if (SocketName == FName("GripRight") && RightHandWeapon == NULL)
	{
		RightHandWeapon = Weapon;
	}
	else if (SocketName == FName("GripLeft") && LeftHandWeapon == NULL)
	{
		LeftHandWeapon = Weapon;
	}
	else
	{
		return false;
	}

	return true;
}
// Copyright Epic Games, Inc. All Rights Reserved.

#include "BattlemageTheEndlessCharacter.h"
#include "../Pickups/BattlemageTheEndlessProjectile.h"
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

ABattlemageTheEndlessCharacter::ABattlemageTheEndlessCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UBMageCharacterMovementComponent>(ACharacter::CharacterMovementComponentName))
{
	bShouldUnCrouch = false;

	SetupCameras();
	SetupCapsule();

	// Init Audio Resource
	static ConstructorHelpers::FObjectFinder<USoundWave> Sound(TEXT("/Game/Sounds/Jump_Landing"));
	JumpLandingSound = Sound.Object;

	JumpMaxCount = 2;
}

void ABattlemageTheEndlessCharacter::SetupCapsule()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(55.f, 96.0f);
	GetCapsuleComponent()->OnComponentBeginOverlap.AddDynamic(this, &ABattlemageTheEndlessCharacter::OnBaseCapsuleBeginOverlap);
}

void ABattlemageTheEndlessCharacter::Destroyed()
{
	Super::Destroyed();

	// Example to bind to OnPlayerDied event in GameMode. 
	if (UWorld* World = GetWorld())
	{
		if (ABattlemageTheEndlessGameMode* GameMode = Cast<ABattlemageTheEndlessGameMode>(World->GetAuthGameMode()))
		{
			GameMode->GetOnPlayerDied().Broadcast(this);
		}
	}
}

void ABattlemageTheEndlessCharacter::CallRestartPlayer()
{
	//Get the Controller of the Character.
	AController* controllerRef = GetController();
	if (LastCheckPoint)
		controllerRef->StartSpot = LastCheckPoint;

	//Destroy the Player.   
	Destroy();

	//Get the World and GameMode in the world to invoke its restart player function.
	if (UWorld* World = GetWorld())
	{
		if (ABattlemageTheEndlessGameMode* GameMode = Cast<ABattlemageTheEndlessGameMode>(World->GetAuthGameMode()))
		{
			GameMode->RestartPlayer(controllerRef);
		}
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

	TObjectPtr<UBMageCharacterMovementComponent> movement = Cast<UBMageCharacterMovementComponent>(GetCharacterMovement());
	if (movement)
	{
		movement->InitAbilities(this, GetMesh());
	}

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
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Triggered, this, &ABattlemageTheEndlessCharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		// Launch Jump
		EnhancedInputComponent->BindAction(LaunchJumpAction, ETriggerEvent::Started, this, &ABattlemageTheEndlessCharacter::LaunchJump);
		EnhancedInputComponent->BindAction(LaunchJumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		// Crouching
		EnhancedInputComponent->BindAction(CrouchAction, ETriggerEvent::Started, this, &ABattlemageTheEndlessCharacter::Crouch);
		EnhancedInputComponent->BindAction(CrouchAction, ETriggerEvent::Completed, this, &ABattlemageTheEndlessCharacter::RequestUnCrouch);

		// Sprinting
		EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Triggered, this, &ABattlemageTheEndlessCharacter::StartSprint);
		EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Completed, this, &ABattlemageTheEndlessCharacter::EndSprint);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ABattlemageTheEndlessCharacter::Move);

		// Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ABattlemageTheEndlessCharacter::Look);

		// Switch Camera
		EnhancedInputComponent->BindAction(SwitchCameraAction, ETriggerEvent::Triggered, this, &ABattlemageTheEndlessCharacter::SwitchCamera);

		// Dodge Actions
		EnhancedInputComponent->BindAction(DodgeAction, ETriggerEvent::Triggered, this, &ABattlemageTheEndlessCharacter::DodgeInput);

		EnhancedInputComponent->BindAction(RespawnAction, ETriggerEvent::Triggered, this, &ABattlemageTheEndlessCharacter::CallRestartPlayer);
	}
	else
	{
		UE_LOG(LogTemplateCharacter, Error, TEXT("'%s' Failed to find an Enhanced Input Component!"), *GetNameSafe(this));
	}
}

void ABattlemageTheEndlessCharacter::Crouch() 
{
	UBMageCharacterMovementComponent* movement = Cast<UBMageCharacterMovementComponent>(GetCharacterMovement());
	if (!movement)
	{
		UE_LOG(LogTemplateCharacter, Error, TEXT("'%s' Failed to find a Movement Component!"), *GetNameSafe(this));
		return;
	}

	// nothing to do if we're already crouching and can't crouch in the air
	if (bIsCrouched || movement->MovementMode == EMovementMode::MOVE_Falling)
		return;

	ACharacter::Crouch(false);

	if (!movement->IsAbilityActive(MovementAbilityType::Sprint) && !movement->IsAbilityActive(MovementAbilityType::Dodge))
	{
		return;
	}

	// Start a slide if we've made it this far
	movement->TryStartAbility(MovementAbilityType::Slide);
}

void ABattlemageTheEndlessCharacter::RequestUnCrouch()
{
	if (!bIsCrouched)
		return;

	bShouldUnCrouch = true;
}

void ABattlemageTheEndlessCharacter::TickActor(float DeltaTime, ELevelTick TickType, FActorTickFunction& ThisTickFunction)
{
	// lots of dependencies on movement in here, just get it since we always need it
	UCharacterMovementComponent* movement = GetCharacterMovement();

	// TODO: See if this needs to be moved
	if (bShouldUnCrouch)
		DoUnCrouch(movement);

	// call super to apply movement before we evaluate a change in z velocity
	AActor::TickActor(DeltaTime, TickType, ThisTickFunction);
}

void ABattlemageTheEndlessCharacter::DoUnCrouch(UCharacterMovementComponent* movement)
{
	UnCrouch(false);
	bShouldUnCrouch = false;

	// get movement component as BMageCharacterMovementComponent
	UBMageCharacterMovementComponent* mageMovement = Cast<UBMageCharacterMovementComponent>(movement);
	// end slide if we're sliding
	mageMovement->TryEndAbility(MovementAbilityType::Slide);
}

void ABattlemageTheEndlessCharacter::EndSlide(UCharacterMovementComponent* movement)
{
	if(UBMageCharacterMovementComponent* mageMovement = Cast<UBMageCharacterMovementComponent>(movement))
		mageMovement->TryEndAbility(MovementAbilityType::Slide);
}

void ABattlemageTheEndlessCharacter::StartSprint()
{
	TObjectPtr<UBMageCharacterMovementComponent> movement = Cast<UBMageCharacterMovementComponent>(GetCharacterMovement());
	if (!movement)
		return;

	movement->TryStartAbility(MovementAbilityType::Sprint);

	if (bIsCrouched)
		UnCrouch();
}

void ABattlemageTheEndlessCharacter::EndSprint()
{
	if (TObjectPtr<UBMageCharacterMovementComponent> movement = Cast<UBMageCharacterMovementComponent>(GetCharacterMovement()))
	{
		movement->TryEndAbility(MovementAbilityType::Sprint);
	}
}

void ABattlemageTheEndlessCharacter::LaunchJump()
{
	// get movement component as BMageCharacterMovementComponent
	if (UBMageCharacterMovementComponent* mageMovement = Cast<UBMageCharacterMovementComponent>(GetCharacterMovement()))
		mageMovement->TryStartAbility(MovementAbilityType::Launch);
}

void ABattlemageTheEndlessCharacter::Move(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		TObjectPtr<UBMageCharacterMovementComponent> movement = Cast<UBMageCharacterMovementComponent>(GetCharacterMovement());
		// add movement
		if (!movement->IsAbilityActive(MovementAbilityType::WallRun))
		{
			AddMovementInput(GetActorForwardVector(), MovementVector.Y);
			AddMovementInput(GetActorRightVector(), MovementVector.X);
		}
		else
		{
			// if we're wall running, we want to apply all movement to the direction the character is facing (instead of the camera)
			// To do that we'll synthesize a world direction vector by rotating the XAxisVector by the character's yaw
			AddMovementInput(FVector::XAxisVector.RotateAngleAxis(GetRootComponent()->GetComponentRotation().Yaw, FVector::ZAxisVector), MovementVector.X);

			// Don't apply lateral movement, only jumping or dodging can break a wallrun			
		}

		movement->ApplyInput();
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

// TODO: Figure out how to handle this with movement abilities
void ABattlemageTheEndlessCharacter::Jump()
{
	// jump cooldown
	milliseconds now = duration_cast<milliseconds>(system_clock::now().time_since_epoch());
	if (now - _lastJumpTime < (JumpCooldown*1000ms))
		return;

	// get movement component as BMageCharacterMovementComponent
	UBMageCharacterMovementComponent* mageMovement = Cast<UBMageCharacterMovementComponent>(GetCharacterMovement());

	_lastJumpTime = now;

	if (mageMovement->IsAbilityActive(MovementAbilityType::Slide))
		mageMovement->TryStartAbility(MovementAbilityType::Launch);

	if (bIsCrouched)
		UnCrouch();

	bool wallrunEnded = false;
	if (mageMovement->IsAbilityActive(MovementAbilityType::WallRun))
	{
		mageMovement->TryEndAbility(MovementAbilityType::WallRun);
		wallrunEnded = true;
	}

	// movement redirection logic
	if (JumpCurrentCount < JumpMaxCount)
		RedirectVelocityToLookDirection(wallrunEnded);

	Super::Jump();

	// this is to get around the double jump check in CheckJumpInput
	if (wallrunEnded)
		JumpCurrentCount--;
}

void ABattlemageTheEndlessCharacter::RedirectVelocityToLookDirection(bool wallrunEnded)
{
	UBMageCharacterMovementComponent* movement = Cast<UBMageCharacterMovementComponent>(GetCharacterMovement());
	if (!movement || movement->MovementMode != EMovementMode::MOVE_Falling)
		return;

	// jumping past apex will have the different gravity scale, reset it to the base
	if (movement->GravityScale != movement->CharacterBaseGravityScale)
		movement->GravityScale = movement->CharacterBaseGravityScale;
	
	UCameraComponent* activeCamera = FirstPersonCamera->IsActive() ? FirstPersonCamera : ThirdPersonCamera;
	// use the already normalized rotation vector as the base
	// NOTE: we are using the camera's rotation instead of the character's rotation to support wall running, which disables camera based character yaw control
	FVector targetDirectionVector = activeCamera->GetComponentRotation().Vector();
	//movement->GetLastUpdateRotation().Vector();

	// we only want to apply the movement input if it hasn't already been consumed
	if (ApplyMovementInputToJump && LastControlInputVector != FVector::ZeroVector && !wallrunEnded)
	{
		FVector movementImpact = LastControlInputVector;
		targetDirectionVector += movementImpact;
	}

	// normalize both rotators to positive rotation so we can safely use them to calculate the yaw difference
	FRotator targetRotator = targetDirectionVector.Rotation();
	FRotator movementRotator = movement->Velocity.Rotation();
	VectorMath::NormalizeRotator0To360(targetRotator);
	VectorMath::NormalizeRotator0To360(movementRotator);

	// calculate the yaw difference
	float yawDifference = targetRotator.Yaw - movementRotator.Yaw;

	// rotate the movement vector to the target direction
	movement->Velocity = movement->Velocity.RotateAngleAxis(yawDifference, FVector::ZAxisVector);	
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
	if (PrevMovementMode == EMovementMode::MOVE_Falling && GetCharacterMovement()->MovementMode == EMovementMode::MOVE_Walking)
	{
		// TODO: Use anim notify instead
		UGameplayStatics::PlaySoundAtLocation(this,
			JumpLandingSound,
			GetActorLocation(), 1.0f);
	}

	ACharacter::OnMovementModeChanged(PrevMovementMode, PreviousCustomMode);
}

// TODO: Can I migrate this to the player controller?
void ABattlemageTheEndlessCharacter::SwitchCamera() 
{
	milliseconds now = duration_cast<milliseconds>(system_clock::now().time_since_epoch());
	if (now - _lastCameraSwap < 500ms)
		return;

	_lastCameraSwap = now;
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

void ABattlemageTheEndlessCharacter::DodgeInput()
{
	// get the movement component as a BMageCharacterMovementComponent
	if (UBMageCharacterMovementComponent* movement = Cast<UBMageCharacterMovementComponent>(GetCharacterMovement()))
		movement->TryStartAbility(MovementAbilityType::Dodge);
}

// TODO: Deprecate this
bool ABattlemageTheEndlessCharacter::IsDodging()
{
	UBMageCharacterMovementComponent* movement = Cast<UBMageCharacterMovementComponent>(GetCharacterMovement());
	return movement && movement->IsAbilityActive(MovementAbilityType::Dodge);
}

void ABattlemageTheEndlessCharacter::ApplyDamage(float damage)
{
	Health -= damage > Health ? Health : damage;
}

FName ABattlemageTheEndlessCharacter::GetTargetSocketName(EquipSlot SlotType)
{
	if (SlotType == EquipSlot::Primary)
		return FName(LeftHanded ? "GripLeft" : "GripRight");
	else
		return FName(LeftHanded ? "GripRight" : "GripLeft");
}

bool ABattlemageTheEndlessCharacter::TrySetWeapon(UTP_WeaponComponent* Weapon, FName SocketName)
{
	if (SocketName == FName("GripRight") && RightHandWeapon == NULL)
		RightHandWeapon = Weapon;
	else if (SocketName == FName("GripLeft") && LeftHandWeapon == NULL)
		LeftHandWeapon = Weapon;
	else
		return false;

	return true;
}

void ABattlemageTheEndlessCharacter::OnBaseCapsuleBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// if the other actor is not a CheckPoint, return
	if (ACheckPoint* checkpoint = Cast<ACheckPoint>(OtherActor))
		LastCheckPoint = checkpoint;
}
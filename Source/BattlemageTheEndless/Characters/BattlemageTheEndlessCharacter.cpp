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


ABattlemageTheEndlessCharacter::ABattlemageTheEndlessCharacter()
{
	// Character doesnt have a rifle at start
	bIsSprinting = false;
	IsSliding = false;
	bShouldUnCrouch = false;
	slideElapsedSeconds = 0.0f;
	launchesPerformed = 0;

	SetupCameras();
	SetupCapsules();

	// Init Audio Resource
	static ConstructorHelpers::FObjectFinder<USoundWave> Sound(TEXT("/Game/Sounds/Jump_Landing"));
	JumpLandingSound = Sound.Object;

	JumpMaxCount = 2;

	MovementAbilities[MovementAbilityType::Launch] = Cast<ULaunchAbility>(ULaunchAbility());

	TObjectPtr<UCharacterMovementComponent> movement = GetCharacterMovement();
	if (movement)
	{
		movement->AirControl = BaseAirControl;
		movement->GetNavAgentPropertiesRef().bCanCrouch = true;
	}

}

void ABattlemageTheEndlessCharacter::SetupCapsules()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(55.f, 96.0f);
	GetCapsuleComponent()->OnComponentHit.AddDynamic(this, &ABattlemageTheEndlessCharacter::OnHit);
	GetCapsuleComponent()->OnComponentBeginOverlap.AddDynamic(this, &ABattlemageTheEndlessCharacter::OnBaseCapsuleBeginOverlap);

	WallRunCapsule = CreateDefaultSubobject<UCapsuleComponent>(TEXT("WallRunCapsule"));
	WallRunCapsule->InitCapsuleSize(55.f, 96.0f);
	WallRunCapsule->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Overlap);
	WallRunCapsule->SetupAttachment(GetCapsuleComponent());
	WallRunCapsule->SetRelativeLocation(FVector(0.f, 0.f, 0.f));
	WallRunCapsule->OnComponentBeginOverlap.AddDynamic(this, &ABattlemageTheEndlessCharacter::OnWallRunCapsuleBeginOverlap);
	WallRunCapsule->OnComponentEndOverlap.AddDynamic(this, &ABattlemageTheEndlessCharacter::OnWallRunCapsuleEndOverlap);
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
	UCharacterMovementComponent* movement = GetCharacterMovement();
	if (!movement)
	{
		UE_LOG(LogTemplateCharacter, Error, TEXT("'%s' Failed to find a Movement Component!"), *GetNameSafe(this));
		return;
	}

	// nothing to do if we're already crouching and can't crouch in the air
	if (bIsCrouched || movement->MovementMode == EMovementMode::MOVE_Falling)
		return;

	ACharacter::Crouch(false);

	if (!bIsSprinting && !IsDodging())
	{
		return;
	}

	// Start a slide if we've made it this far
	IsSliding = true;
	movement->SetCrouchedHalfHeight(SlideHalfHeight);
	// default walk is 1200, crouch is 300
	movement->MaxWalkSpeedCrouched = SprintSpeed;
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

	if (IsSliding)
		TickSlide(DeltaTime, movement);

	if (bShouldUnCrouch)
		DoUnCrouch(movement);

	if (IsVaulting)
		TickVault(DeltaTime);

	if (IsWallRunning)
		TickWallRun(movement, DeltaTime);

	// call super to apply movement before we evaluate a change in z velocity
	AActor::TickActor(DeltaTime, TickType, ThisTickFunction);

	// if we're past the apex, apply the falling gravity scale
	// ignore this clause if we're wall running
	if (movement && PreviousVelocity.Z >= 0.0f && movement->Velocity.Z < -0.00001f && !IsWallRunning)
	{
		// this is undone in OnMovementModeChanged when the character lands
		movement->GravityScale = CharacterPastJumpApexGravityScale;
	}

	PreviousVelocity = GetCharacterMovement()->Velocity;
}

void ABattlemageTheEndlessCharacter::TickWallRun(UCharacterMovementComponent* movement, float DeltaTime)
{
	// end conditions for wall run
	//	no need to check time since there's a timer running for that
	//	if we are on the ground, end the wall run
	//	if a raycast doesn't find the wall, end the wall run
	if (movement->MovementMode == EMovementMode::MOVE_Walking || !WallRunContinuationRayCast())
	{
		EndWallRun();
	}
	else // if we're still wall running
	{
		// increase gravity linearly based on time elapsed
		movement->GravityScale += (DeltaTime / (WallRunMaxDuration - WallRunGravityDelay)) * (CharacterBaseGravityScale - WallRunInitialGravityScale);

		// if gravity is over CharacterBaseGravityScale, set it to CharacterBaseGravityScale and end the wall run
		if (movement->GravityScale > CharacterBaseGravityScale)
		{
			movement->GravityScale = CharacterBaseGravityScale;
			EndWallRun();
		}
	}
}

void ABattlemageTheEndlessCharacter::TickVault(float DeltaTime)
{
	FVector socketLocationToUse;
	FVector relativeLocationToAdd = FVector::Zero();
	USkeletalMeshComponent* mesh = GetMesh();

	// If a foot is planted, check which one is higher
	if (VaultFootPlanted)
	{
		// check whether a foot is on top of the vaulted object
		FVector socketLocationLeftFoot = mesh->GetSocketLocation(FName("foot_l_Socket"));
		FVector socketLocationRightFoot = mesh->GetSocketLocation(FName("foot_r_Socket"));

		if (VaultAttachPoint.Z < socketLocationLeftFoot.Z)
		{
			socketLocationToUse = socketLocationLeftFoot;
		}
		else
		{
			socketLocationToUse = socketLocationRightFoot;
		}

		// if the foot is planted, move the character forward
		float timeRemaining = VaultDurationSeconds - VaultElapsedTimeBeforeFootPlanted;
		FVector forwardMotionToAdd = FVector(VaultEndForwardDistance * (DeltaTime / timeRemaining), 0.f, 0.f);
		relativeLocationToAdd += forwardMotionToAdd.RotateAngleAxis(GetActorRotation().Yaw, FVector::ZAxisVector);
	}
	// otherwise, use the hand that is higher
	else
	{
		VaultElapsedTimeBeforeFootPlanted += DeltaTime;

		FVector socketLocationLeftHand = mesh->GetSocketLocation(FName("GripLeft"));
		FVector socketLocationRightHand = mesh->GetSocketLocation(FName("GripRight"));

		if (VaultAttachPoint.Z < socketLocationLeftHand.Z)
		{
			socketLocationToUse = socketLocationLeftHand;
		}
		else
		{
			socketLocationToUse = socketLocationRightHand;
		}
	}

	// adjust the character's location based on difference between socket location and attach point
	relativeLocationToAdd.Z = FMath::Max(0.f, (VaultAttachPoint - socketLocationToUse).Z);
	GetRootComponent()->AddRelativeLocation(relativeLocationToAdd);
}

void ABattlemageTheEndlessCharacter::DoUnCrouch(UCharacterMovementComponent* movement)
{
	UnCrouch(false);
	bShouldUnCrouch = false;

	if (IsSliding)
	{
		EndSlide(movement);
	}
	if (bLaunchRequested)
	{
		DoLaunchJump();
		bLaunchRequested = false;
	}
}

void ABattlemageTheEndlessCharacter::TickSlide(float DeltaTime, UCharacterMovementComponent* movement)
{
	slideElapsedSeconds += DeltaTime;

	if (!movement)
		return;
	
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

bool ABattlemageTheEndlessCharacter::WallRunContinuationRayCast()
{
	// Raycast from vaultRaycastSocket straight forward to see if Object is in the way
	FVector start = GetMesh()->GetSocketLocation(FName("feetRaycastSocket"));
	// Cast a ray out in hit normal direction 100 units long
	FVector castVector = (FVector::XAxisVector * 100).RotateAngleAxis(WallRunHit.ImpactNormal.RotateAngleAxis(180.f, FVector::ZAxisVector).Rotation().Yaw, FVector::ZAxisVector);
	FVector end = start + castVector;

	// Perform the raycast
	FHitResult hit;
	FCollisionQueryParams params;
	FCollisionObjectQueryParams objectParams;
	params.AddIgnoredActor(this);
	GetWorld()->LineTraceSingleByObjectType(hit, start, end, objectParams, params);

	DrawDebugLine(GetWorld(), start, end, FColor::Red, false, 1.0f, 0, 1.0f);

	// If the camera raycast did not hit the object, we are too high to wall run
	return hit.GetActor() == WallRunObject;
}

void ABattlemageTheEndlessCharacter::EndSlide(UCharacterMovementComponent* movement)
{
	movement->MaxWalkSpeedCrouched = bIsCrouched ? CrouchSpeed : WalkSpeed;
	IsSliding = false;
	slideElapsedSeconds = 0.0f;
	movement->SetCrouchedHalfHeight(CrouchedHalfHeight);
}

void ABattlemageTheEndlessCharacter::StartSprint()
{
	if (bIsSprinting)
		return;

	TObjectPtr<UCharacterMovementComponent> movement = GetCharacterMovement();
	if (!movement)
		return;

	// if sliding, end it before sprinting
	if (IsSliding)
	{
		EndSlide(movement);
		UnCrouch();
	}

	movement->MaxWalkSpeed = SprintSpeed;
	bIsSprinting = true;
}

void ABattlemageTheEndlessCharacter::EndSprint()
{
	if (!bIsSprinting)
		return;

	TObjectPtr<UCharacterMovementComponent> movement = GetCharacterMovement();
	if (!movement)
		return;

	movement->MaxWalkSpeed = WalkSpeed;
	bIsSprinting = false;
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
		if (!IsWallRunning)
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
			movement->MaxWalkSpeed = bIsSprinting ? SprintSpeed: WalkSpeed;
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
	// jump cooldown
	milliseconds now = duration_cast<milliseconds>(system_clock::now().time_since_epoch());
	if (now - _lastJumpTime < (JumpCooldown*1000ms))
		return;

	_lastJumpTime = now;

	if (IsSliding)
	{
		EndSlide(GetCharacterMovement());
		LaunchJump();
	}

	if (bIsCrouched)
	{
		UnCrouch();
	}

	bool wallrunEnded = false;
	if (IsWallRunning)
	{
		EndWallRun();
		wallrunEnded = true;
	}

	// movement redirection logic
	if (JumpCurrentCount < JumpMaxCount)
	{
		RedirectVelocityToLookDirection(wallrunEnded);

	}

	Super::Jump();
	// this is to get around the double jump check in CheckJumpInput
	if (wallrunEnded)
	{
		JumpCurrentCount--;
	}
}

void ABattlemageTheEndlessCharacter::RedirectVelocityToLookDirection(bool wallrunEnded)
{
	UCharacterMovementComponent* movement = GetCharacterMovement();
	if (!movement || movement->MovementMode != EMovementMode::MOVE_Falling)
		return;

	// jumping past apex will have the different gravity scale, reset it to the base
	if (movement->GravityScale != CharacterBaseGravityScale)
		movement->GravityScale = CharacterBaseGravityScale;
	
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
		OnJumpLanded();

	} else if (PrevMovementMode == EMovementMode::MOVE_Walking && GetCharacterMovement()->MovementMode == EMovementMode::MOVE_Falling)
	{
		TryBeginWallrun();
	}

	ACharacter::OnMovementModeChanged(PrevMovementMode, PreviousCustomMode);
}

void ABattlemageTheEndlessCharacter::TryBeginWallrun()
{
	// check if there are any eligible wallrun objects
	TArray<AActor*> overlappingActors;
	GetOverlappingActors(overlappingActors, nullptr);
	for (AActor* actor : overlappingActors)
	{
		if (ObjectIsWallRunnable(actor))
		{
			WallRunObject = actor;
			// WallRunHit is set in ObjectIsWallRunnable
			WallRun();
			break;
		}
	}
}

void ABattlemageTheEndlessCharacter::OnJumpLanded()
{
	// TODO: Should I be setting current jump count to 0 or does the super handle that?

	launchesPerformed = 0;
	bLaunchRequested = false;
	UGameplayStatics::PlaySoundAtLocation(this,
		JumpLandingSound,
		GetActorLocation(), 1.0f);

	GetCharacterMovement()->GravityScale = CharacterBaseGravityScale;
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

void ABattlemageTheEndlessCharacter::DodgeInput()
{
	FVector inputVector = GetLastMovementInputVector();
	// account for camera rotation
	inputVector = inputVector.RotateAngleAxis(GetCharacterMovement()->GetLastUpdateRotation().GetInverse().Yaw, FVector::ZAxisVector);

	// use the strongest input direction to decide which way to dodge
	bool isLateral = FMath::Abs(inputVector.X) < FMath::Abs(inputVector.Y);
	// if the movement is not lateral, dodge forward or backward
	if (!isLateral)
	{
		// If the input is forward, dodge forward
		if (inputVector.X > 0.0f)
		{
			Dodge(DodgeImpulseForward);
		}
		// If the input is backward, dodge backward
		else if (inputVector.X < -0.0f)
		{
			Dodge(DodgeImpulseBackward);
		}

		return;
	}
	
	// if the input is left dodge left, if right dodge right
	if (inputVector.Y < 0.0f)
	{
		Dodge(DodgeImpulseLateral);
	}
	else if (inputVector.Y > 0.0f)
	{
		Dodge(DodgeImpulseLateral * -1);
	}
}

void ABattlemageTheEndlessCharacter::Dodge(FVector Impulse)
{
	// TODO: Implement a dodge cooldown
	// TODO: Figure out what I want to do with dodging in air
	// Currently only allow dodging if on ground
	UCharacterMovementComponent* movement = GetCharacterMovement();
	if (movement->MovementMode == EMovementMode::MOVE_Falling)
		return;

	// If we're sliding, end it before dodging
	if (IsSliding)
		EndSlide(movement);

	// If we're crouched, uncrouch before dodging
	if (bIsCrouched)
		RequestUnCrouch();

	// Launch the character
	PreviousFriction = movement->GroundFriction;
	movement->GroundFriction = 0.0f;
	FVector dodgeVector = FVector(Impulse.RotateAngleAxis(movement->GetLastUpdateRotation().Yaw, FVector::ZAxisVector));
	LaunchCharacter(dodgeVector, true, true);
	GetWorldTimerManager().SetTimer(DodgeEndTimer, this, &ABattlemageTheEndlessCharacter::RestoreFriction, DodgeDurationSeconds, false);
}

bool ABattlemageTheEndlessCharacter::IsDodging()
{
	return GetWorldTimerManager().IsTimerActive(DodgeEndTimer);
}

void ABattlemageTheEndlessCharacter::RestoreFriction()
{
	UCharacterMovementComponent* movement = GetCharacterMovement();
	movement->GroundFriction = PreviousFriction;
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

void ABattlemageTheEndlessCharacter::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	// We aren't checking for wallrun state since a vault should be able to override a wallrun
	if (!CanVault() || !ObjectIsVaultable(OtherActor))
		return;

	VaultTarget = OtherActor;
	VaultHit = Hit;
	Vault();

	if(IsWallRunning)
		EndWallRun();
}

bool ABattlemageTheEndlessCharacter::CanVault()
{
	// Vaulting is only possible if enabled and in the air
	return bCanVault && GetCharacterMovement()->MovementMode == EMovementMode::MOVE_Falling;
}

bool ABattlemageTheEndlessCharacter::ObjectIsVaultable(AActor* Object)
{
	// if the object is a pawn, was cannot vault it
	if (Object->IsA(APawn::StaticClass()))
		return false;

	bool drawTrace = false;

	// Raycast from cameraSocket straight forward to see if Object is in the way	
	// TODO: Why is this requiring me to pass in optional params?
	FHitResult hit = LineTraceMovementVector(FName("cameraSocket"), 50, drawTrace, FColor::Green, 0.f);

	// If the camera raycast hit the object, we are too low to vault
	if (hit.GetActor() == Object)
		return false;

	// Repeat the same process but use socket vaultRaycastSocket
	hit = LineTraceMovementVector(FName("vaultRaycastSocket"), 50, drawTrace, FColor::Green, 0.f);

	// If the vault raycast hit the object, we can vault
	return hit.GetActor() == Object;
}

void ABattlemageTheEndlessCharacter::Vault()
{
	IsVaulting = true;
	DisableInput(Cast<APlayerController>(Controller));
	// stop movement and gravity by enabling flying
	UCharacterMovementComponent* movement = GetCharacterMovement();
	movement->StopMovementImmediately();
	movement->SetMovementMode(MOVE_Flying);

	FRotator targetDirection = VaultHit.ImpactNormal.RotateAngleAxis(180.f, FVector::ZAxisVector).Rotation();

	// Rotate the character to directly face the vaulted object
	Controller->SetControlRotation(targetDirection);

	// determine the highest point on the vaulted face
	// NOTE: This logic will only work for objects with flat tops
	VaultAttachPoint = VaultHit.ImpactPoint;
	// Box extent seems to be distance from origin (so we're not halving it)
	VaultAttachPoint.Z = VaultHit.Component->Bounds.Origin.Z + VaultHit.Component->Bounds.BoxExtent.Z;

	//DrawDebugSphere(GetWorld(), VaultAttachPoint, 10.0f, 12, FColor::Green, false, 1.0f, 0, 1.0f);

	// check which hand is higher
	FVector socketLocationLeftHand = GetMesh()->GetSocketLocation(FName("GripLeft"));
	FVector socketLocationRightHand = GetMesh()->GetSocketLocation(FName("GripRight"));
	FVector handToUse = socketLocationLeftHand.Z > socketLocationRightHand.Z ? socketLocationLeftHand : socketLocationRightHand;

	GetRootComponent()->AddRelativeLocation(FVector(0.f, 0.f, VaultAttachPoint.Z - handToUse.Z));

	// Disable collision to allow proper movement
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Ignore);

	// anim graph handles the actual movement with a root motion animation to put the character in the right place
	// register a timer to end the vaulting state
	GetWorldTimerManager().SetTimer(VaultEndTimer, this, &ABattlemageTheEndlessCharacter::EndVault, VaultDurationSeconds, false);
}

void ABattlemageTheEndlessCharacter::EndVault()
{
	EnableInput(Cast<APlayerController>(Controller));
	IsVaulting = false;
	GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	VaultTarget = NULL;
	// anim graph handles transition back to normal state
	VaultFootPlanted = false;
	VaultElapsedTimeBeforeFootPlanted = 0;
	// turn collision with worldstatic back on
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	GetMesh()->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
}

void ABattlemageTheEndlessCharacter::OnWallRunCapsuleBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	/*if (GEngine)
		GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Green, FString::Printf(TEXT("'%s' Began overlap"), *AActor::GetDebugName(OtherActor)));*/
	if (CanWallRun() && ObjectIsWallRunnable(OtherActor))
	{
		WallRunObject = OtherActor;
		// WallRunHit is set in ObjectIsWallRunnable
		WallRun();
	}
}

void ABattlemageTheEndlessCharacter::OnWallRunCapsuleEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (OtherActor == WallRunObject)
		EndWallRun();
}

void ABattlemageTheEndlessCharacter::OnBaseCapsuleBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// if the other actor is not a CheckPoint, return
	if (ACheckPoint* checkpoint = Cast<ACheckPoint>(OtherActor))
	{
		LastCheckPoint = checkpoint;
	}
}

bool ABattlemageTheEndlessCharacter::CanWallRun()
{	
	return bIsSprinting && GetCharacterMovement()->MovementMode == EMovementMode::MOVE_Falling;
}

FHitResult ABattlemageTheEndlessCharacter::LineTraceMovementVector(FName socketName, float magnitude, bool drawTrace = false, FColor drawColor = FColor::Green, float rotateYawByDegrees = 0.f)
{
	FVector start = GetMesh()->GetSocketLocation(socketName);
	// Cast a ray out in look direction magnitude units long
	FVector castVector = (FVector::XAxisVector * magnitude).RotateAngleAxis(GetCharacterMovement()->GetLastUpdateRotation().Yaw+rotateYawByDegrees, FVector::ZAxisVector);
	FVector end = start + castVector;

	// Perform the raycast
	FHitResult hit = LineTraceGeneric(start, end);

	if (drawTrace)
		DrawDebugLine(GetWorld(), start, end, drawColor, false, 3.0f, 0, 1.0f);
	return hit;
}

FHitResult ABattlemageTheEndlessCharacter::LineTraceGeneric(FVector start, FVector end)
{
	// Perform the raycast
	FHitResult hit;
	FCollisionQueryParams params;
	FCollisionObjectQueryParams objectParams;
	params.AddIgnoredActor(this);
	GetWorld()->LineTraceSingleByObjectType(hit, start, end, objectParams, params);
	return hit;
}

bool ABattlemageTheEndlessCharacter::ObjectIsWallRunnable(AActor* Object)
{
	// if the object is a pawn, was cannot wallrun on it
	if (Object->IsA(APawn::StaticClass()))
		return false;

	bool drawTrace = true;

	// Repeat the same process but use socket feetRaycastSocket
	FHitResult hit = LineTraceMovementVector(FName("feetRaycastSocket"), 500, drawTrace);

	bool hitLeft = false;
	bool hitRight = false;
	// If we didn't hit the object, try again with a vector 45 degress left
	if (hit.GetActor() != Object)
	{
		hit = LineTraceMovementVector(FName("feetRaycastSocket"), 500, drawTrace, FColor::Blue, -45.f);

		hitLeft = hit.GetActor() == Object;
		// if we still didn't hit, try 45 right
		if(!hitLeft)
		{
			hit = LineTraceMovementVector(FName("feetRaycastSocket"), 500, drawTrace, FColor::Emerald, 45.f);
			hitRight = hit.GetActor() == Object;
		}

		// We have now tried all supported directions, if we didn't hit the object, we can't wall run
		if (!hitLeft && !hitRight)
			return false;
	}		

	FVector impactDirection = hit.ImpactNormal.RotateAngleAxis(180.f, FVector::ZAxisVector);
	float yawDifference = VectorMath::Vector2DRotationDifference(GetCharacterMovement()->Velocity, impactDirection);

	// there is no need to correct for the left or right hit cases since the impact normal will be the same

	//if (GEngine)
	//	GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Green, FString::Printf(TEXT("'%f' yaw diff"), yawDifference));

	// wall run if we're more than 20 degrees rotated from the wall but less than 
	if (yawDifference > 20.f && yawDifference < 180.f)
	{
		/*if (GEngine)
			GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Green, TEXT("Starting Wall Run"));*/
		WallRunHit = hit;
		return true;	
	}
	return false;
}

void ABattlemageTheEndlessCharacter::WallRun()
{
	IsWallRunning = true;
	GetCharacterMovement()->GravityScale = WallRunInitialGravityScale;

	// Wall running refunds a jump charge
	if (JumpCurrentCount > 0)
	{
		JumpCurrentCount = FMath::Max(0, JumpCurrentCount-WallRunJumpRefundCount);
	}

	UCharacterMovementComponent* movement = GetCharacterMovement();

	// This is used by the anim graph to determine which way to mirror the wallrun animation
	FVector start = GetMesh()->GetSocketLocation(FName("feetRaycastSocket"));
	// add 30 degrees to the left of the forward vector to account for wall runs starting near the start of the wall
	FVector end = start + FVector::LeftVector.RotateAngleAxis(GetRootComponent()->GetComponentRotation().Yaw+30.f, FVector::ZAxisVector) * 200;
	WallIsToLeft = LineTraceGeneric(start, end).GetActor() == WallRunObject;

	// get vectors parallel to the wall
	FRotator possibleWallRunDirectionOne = WallRunHit.ImpactNormal.RotateAngleAxis(90.f, FVector::ZAxisVector).Rotation();
	FRotator possibleWallRunDirectionTwo = WallRunHit.ImpactNormal.RotateAngleAxis(-90.f, FVector::ZAxisVector).Rotation();

	float dirOne360 = VectorMath::NormalizeRotator0To360(possibleWallRunDirectionOne).Yaw;
	float dirTwo360 = VectorMath::NormalizeRotator0To360(possibleWallRunDirectionTwo).Yaw;
	float dirOne180 = VectorMath::NormalizeRotator180s(possibleWallRunDirectionOne).Yaw;
	float dirTwo180 = VectorMath::NormalizeRotator180s(possibleWallRunDirectionTwo).Yaw;

	float lookDirection = movement->GetLastUpdateRotation().Yaw;

	// find the least yaw difference
	float closestDir = dirOne360;
	if (FMath::Abs(dirTwo360 - lookDirection) < FMath::Abs(closestDir - lookDirection))
	{
		closestDir = dirTwo360;
	}
	if(FMath::Abs(dirOne180 - lookDirection) < FMath::Abs(closestDir - lookDirection))
	{
		closestDir = dirOne180;
	}
	if (FMath::Abs(dirTwo180 - lookDirection) < FMath::Abs(closestDir - lookDirection))
	{
		closestDir = dirTwo180;
	}
	FRotator targetRotation = FRotator(0.f, closestDir, 0.f);
	Controller->SetControlRotation(targetRotation);

	/*if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Green, 
			FString::Printf(TEXT("Wallrun Start Direction: %s, Is Wall Left? %s"), 
				*targetRotation.ToString(), WallIsToLeft ? TEXT("true") : TEXT("false")));
	}*/
	
	// redirect character's velocity to be parallel to the wall, ignore input
	movement->Velocity = targetRotation.Vector() * movement->Velocity.Size();
	// kill any vertical movement
	movement->Velocity.Z = 0.f;
	
	// disable player rotation from input
	bUseControllerRotationYaw = false;
	
	// set max pan angle to 60 degrees
	if (APlayerCameraManager* cameraManager = UGameplayStatics::GetPlayerCameraManager(GetWorld(), 0))
	{
		float currentYaw = targetRotation.Yaw;
		cameraManager->ViewYawMax = currentYaw + 90.f;
		cameraManager->ViewYawMin = currentYaw - 90.f;
	}
	
	// set air control to 100% to allow for continued movement parallel to the wall
	movement->AirControl = 1.0f;

	// set a timer to end the wall run
	GetWorldTimerManager().SetTimer(WallRunTimer, this, &ABattlemageTheEndlessCharacter::EndWallRun, WallRunMaxDuration, false);
}

void ABattlemageTheEndlessCharacter::EndWallRun()
{
	// This can be called before the timer goes off, so check if we're actually wall running
	if (!IsWallRunning && GetCharacterMovement()->GravityScale == CharacterBaseGravityScale && bUseControllerRotationYaw)
		return;

	/*if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Green, FString::Printf(TEXT("Start of EndWallRun ForwardVector: %s"), *GetActorForwardVector().ToString()));
	}*/

	// reset air control to default
	GetCharacterMovement()->AirControl = BaseAirControl;

	IsWallRunning = false;
	GetCharacterMovement()->GravityScale = CharacterBaseGravityScale;
	WallRunObject = NULL;

	// re-enable player rotation from input
	bUseControllerRotationYaw = true;

	// set max pan angle to 60 degrees
	if (APlayerCameraManager* cameraManager = UGameplayStatics::GetPlayerCameraManager(GetWorld(), 0))
	{
		float currentYaw = Controller->GetControlRotation().Yaw;
		cameraManager->ViewYawMax = 359.998993f;
		cameraManager->ViewYawMin = 0.f;
	}

	/*if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Green, FString::Printf(TEXT("End of EndWallRun ForwardVector: %s"), *GetActorForwardVector().ToString()));
	}*/
}
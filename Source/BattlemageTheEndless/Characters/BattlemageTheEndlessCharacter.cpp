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
	bShouldUncrouch = false;
	slideElapsedSeconds = 0.0f;
	launchesPerformed = 0;

	SetupCameras();

	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(55.f, 96.0f);
	GetCapsuleComponent()->OnComponentHit.AddDynamic(this, &ABattlemageTheEndlessCharacter::OnHit);

	WallRunCapsule = CreateDefaultSubobject<UCapsuleComponent>(TEXT("WallRunCapsule"));
	WallRunCapsule->InitCapsuleSize(55.f, 96.0f);
	WallRunCapsule->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Overlap);
	WallRunCapsule->SetupAttachment(GetCapsuleComponent());
	WallRunCapsule->SetRelativeLocation(FVector(0.f, 0.f, 0.f));
	WallRunCapsule->OnComponentBeginOverlap.AddDynamic(this, &ABattlemageTheEndlessCharacter::OnWallRunCapsuleBeginOverlap);
	WallRunCapsule->OnComponentEndOverlap.AddDynamic(this, &ABattlemageTheEndlessCharacter::OnWallRunCapsuleEndOverlap);

	// Init Audio Resource
	static ConstructorHelpers::FObjectFinder<USoundWave> Sound(TEXT("/Game/Sounds/Jump_Landing"));
	JumpLandingSound = Sound.Object;

	JumpMaxCount = 2;

	TObjectPtr<UCharacterMovementComponent> movement = GetCharacterMovement();
	if (movement)
	{
		movement->AirControl = BaseAirControl;
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

	if (!bIsSprinting)
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

	bShouldUncrouch = true;
}

void ABattlemageTheEndlessCharacter::TickActor(float DeltaTime, ELevelTick TickType, FActorTickFunction& ThisTickFunction)
{
	// If we're sliding, handle it
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

	if (IsVaulting)
	{
		FVector socketLocationToUse;
		FVector relativeLocationToAdd = FVector::Zero();

		// If a foot is planted, check which one is higher
		if (VaultFootPlanted)
		{
			// check whether a foot is on top of the vaulted object
			FVector socketLocationLeftFoot = GetMesh()->GetSocketLocation(FName("foot_l_Socket"));
			FVector socketLocationRightFoot = GetMesh()->GetSocketLocation(FName("foot_r_Socket"));

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

			FVector socketLocationLeftHand = GetMesh()->GetSocketLocation(FName("GripLeft"));
			FVector socketLocationRightHand = GetMesh()->GetSocketLocation(FName("GripRight"));

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
	if (IsWallRunning)
	{
		UCharacterMovementComponent* movement = GetCharacterMovement();
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
			movement->GravityScale += (DeltaTime / (WallRunMaxDuration - WallRunGravityDelay)) * (CharacterBaseGravityScale - WallRunInitialGravitScale);

			// if gravity is over CharacterBaseGravityScale, set it to CharacterBaseGravityScale and end the wall run
			if (movement->GravityScale > CharacterBaseGravityScale)
			{
				movement->GravityScale = CharacterBaseGravityScale;
				EndWallRun();
			}
		}


	}
	AActor::TickActor(DeltaTime, TickType, ThisTickFunction);
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
	/*if (GEngine)
		GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Green, FString::Printf(TEXT("'%i' Jump Triggered"), JumpCurrentCount));*/

	// jump cooldown
	milliseconds now = duration_cast<milliseconds>(system_clock::now().time_since_epoch());
	if (now - _lastJumpTime < (JumpCooldown*1000ms))
		return;

	_lastJumpTime = now;

	if (IsSliding)
	{
		EndSlide(GetCharacterMovement());
	}

	if (bIsCrouched)
	{
		UnCrouch();
	}

	if (IsWallRunning)
	{
		EndWallRun();
	}

	if (JumpCurrentCount < JumpMaxCount)
	{
		UCharacterMovementComponent* movement = GetCharacterMovement();
		if (movement && movement->MovementMode == EMovementMode::MOVE_Falling)
		{
			// redirect the character's current velocity in the direction they're facing
			float yawDifference = GetBaseAimRotation().Yaw - movement->Velocity.Rotation().Yaw;

			// get rotation of last input vector
			FRotator inputRotator = LastControlInputVector.RotateAngleAxis(movement->GetLastUpdateRotation().GetInverse().Yaw, FVector::ZAxisVector).Rotation();

			// rotate the velocity vector to match the character's facing direction, accounting for input rotation
			movement->Velocity = movement->Velocity.RotateAngleAxis(inputRotator.Yaw, FVector::ZAxisVector);
		}

		Super::Jump();
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
	if (PrevMovementMode == EMovementMode::MOVE_Falling && GetCharacterMovement()->MovementMode == EMovementMode::MOVE_Walking)
	{
		launchesPerformed = 0;
		bLaunchRequested = false;
		UGameplayStatics::PlaySoundAtLocation(this,
			JumpLandingSound,
			GetActorLocation(), 1.0f);
	} else if (PrevMovementMode == EMovementMode::MOVE_Walking && GetCharacterMovement()->MovementMode == EMovementMode::MOVE_Falling)
	{
		// check if there are any eligible wallrun objects
		TArray<AActor*> overlappingActors;
		GetOverlappingActors(overlappingActors, nullptr);
		for(AActor* actor: overlappingActors)
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

	ACharacter::OnMovementModeChanged(PrevMovementMode, PreviousCustomMode);
}

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
	{
		return;
	}
	// If we're sliding, end it before dodging
	if (IsSliding)
	{
		EndSlide(movement);
	}

	// If we're crouched, uncrouch before dodging
	if (bIsCrouched)
	{
		RequestUnCrouch();
	}

	// Launch the character
	PreviousFriction = movement->GroundFriction;
	movement->GroundFriction = 0.0f;
	FVector dodgeVector = FVector(Impulse.RotateAngleAxis(movement->GetLastUpdateRotation().Yaw, FVector::ZAxisVector));
	LaunchCharacter(dodgeVector, true, true);
	GetWorldTimerManager().SetTimer(DodgeEndTimer, this, &ABattlemageTheEndlessCharacter::RestoreFriction, DodgeDurationSeconds, false);
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
	if (CanVault() && ObjectIsVaultable(OtherActor))
	{
		VaultTarget = OtherActor;
		VaultHit = Hit;
		Vault();
		if(IsWallRunning)
		{
			EndWallRun();
		}
	}
}

bool ABattlemageTheEndlessCharacter::CanVault()
{
	// Vaulting is only possible if enabled (this might get more complicated later)
	return bCanVault;
}

// TODO: use LineTraceMovementVector
bool ABattlemageTheEndlessCharacter::ObjectIsVaultable(AActor* Object)
{
	// Raycast from cameraSocket straight forward to see if Object is in the way
	FVector start = GetMesh()->GetSocketLocation(FName("cameraSocket"));
	// Cast a ray out in look direction 10 units long
	FVector castVector = (FVector::XAxisVector * 50).RotateAngleAxis(GetCharacterMovement()->GetLastUpdateRotation().Yaw, FVector::ZAxisVector);
	FVector end = start + castVector;

	// Perform the raycast
	FHitResult hit;
	FCollisionQueryParams params;
	FCollisionObjectQueryParams objectParams;
	params.AddIgnoredActor(this);
	GetWorld()->LineTraceSingleByObjectType(hit, start, end, objectParams, params);

	//DrawDebugLine(GetWorld(), start, end, FColor::Red, false, 1.0f, 0, 1.0f);

	// If the camera raycast hit the object, we are too low to vault
	if (hit.GetActor() == Object)
		return false;

	// Repeat the same process but use socket vaultRaycastSocket
	start = GetMesh()->GetSocketLocation(FName("vaultRaycastSocket"));
	end = start + castVector;
	GetWorld()->LineTraceSingleByObjectType(hit, start, end, objectParams, params);

	//DrawDebugLine(GetWorld(), start, end, FColor::Green, false, 1.0f, 0, 1.0f);

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

	DrawDebugSphere(GetWorld(), VaultAttachPoint, 10.0f, 12, FColor::Green, false, 1.0f, 0, 1.0f);

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
	bool drawTrace = true;
	/* Don't think I care about this for now, we wouldn't be trying to wallrun during a vault anyway
	// Raycast from vaultRaycastSocket straight forward to see if Object is in the way
	FHitResult hit = LineTraceMovementVector(FName("vaultRaycastSocket"), 500, drawTrace, FColor::Red);

	// If the camera raycast did not hit the object, we are too high to wall run
	if (hit.GetActor() != Object)
		return false;
	*/

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
	float yawDifference = 90.f - VectorMath::Vector2DRotationDifference(GetCharacterMovement()->Velocity, impactDirection);

	// there is no need to correct for the left or right hit cases since the impact normal will be the same

	if (GEngine)
		GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Green, FString::Printf(TEXT("'%f' yaw diff"), yawDifference));

	if (yawDifference <= 60)
	{
		if (GEngine)
			GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Green, TEXT("Starting Wall Run"));
		WallRunHit = hit;
		return true;	
	}
	return false;
}

void ABattlemageTheEndlessCharacter::WallRun()
{
	IsWallRunning = true;
	GetCharacterMovement()->GravityScale = WallRunInitialGravitScale;

	// Wall running refunds a jump charge
	if (JumpCurrentCount > 0)
	{
		JumpCurrentCount = FMath::Max(0, JumpCurrentCount-WallRunJumpRefundCount);
	}

	UCharacterMovementComponent* movement = GetCharacterMovement();

	FVector start = GetMesh()->GetSocketLocation(FName("feetRaycastSocket"));
	FVector end = start + FVector::LeftVector.RotateAngleAxis(GetRootComponent()->GetComponentRotation().Yaw, FVector::ZAxisVector) * 200;
	WallIsToLeft = LineTraceGeneric(start, end).GetActor() == WallRunObject;

	// get vectors parallel to the wall
	FRotator possibleWallRunDirectionOne = WallRunHit.ImpactNormal.RotateAngleAxis(90.f, FVector::ZAxisVector).Rotation();
	FRotator possibleWallRunDirectionTwo = WallRunHit.ImpactNormal.RotateAngleAxis(-90.f, FVector::ZAxisVector).Rotation();
	FRotator lookDirection = movement->GetLastUpdateRotation();

	int normalizedLookYaw = ((int)lookDirection.Yaw + 360) % 360;
	bool isQuadrant3 = normalizedLookYaw > 180 && normalizedLookYaw < 270;

	// Special case to treat 0 and 360 as the same value
	if (isQuadrant3 && FMath::Abs((double)possibleWallRunDirectionTwo.Yaw) < 0.00001)
		possibleWallRunDirectionTwo.Yaw = 360.f;

	// Normalize rotations to be between 0 and 360
	// NOTE: There is a rotator normalization function built into ue but it seems to be doing nothing
	//VectorMath::NormalizeRotator(possibleWallRunDirectionOne);
	//VectorMath::NormalizeRotator(possibleWallRunDirectionTwo);
	//VectorMath::NormalizeRotator(lookDirection);

	// determine which direction is closer to the character's current look direction
	float yawDiffOne = possibleWallRunDirectionOne.Yaw - lookDirection.Yaw;
	float yawDiffTwo = possibleWallRunDirectionTwo.Yaw - lookDirection.Yaw;
	bool isOneLess = FMath::Abs(yawDiffOne) <= FMath::Abs(yawDiffTwo);
	FRotator targetRotation = isOneLess ? possibleWallRunDirectionOne : possibleWallRunDirectionTwo;

	Controller->SetControlRotation(targetRotation);
	
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
}
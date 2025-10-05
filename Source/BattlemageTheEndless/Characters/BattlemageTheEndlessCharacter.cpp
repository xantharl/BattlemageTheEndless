// Copyright Epic Games, Inc. All Rights Reserved.

#include "BattlemageTheEndlessCharacter.h"
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
	SetupCameras();
	SetupCapsule();

	// Init Audio Resource
	static ConstructorHelpers::FObjectFinder<USoundWave> Sound(TEXT("/Game/Sounds/Jump_Landing"));
	JumpLandingSound = Sound.Object;

	JumpMaxCount = 2;

	// init gas
	AbilitySystemComponent = CreateDefaultSubobject<UBMageAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);

	// See details of replication modes https://github.com/tranek/GASDocumentation?tab=readme-ov-file#concepts-asc-rm
	// Defaulting to minimal, updates to Mixed on possession (which indicates this is a player character)
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);
	AbilitySystemComponent->OnAnyGameplayEffectRemovedDelegate().AddUObject(this, &ABattlemageTheEndlessCharacter::OnRemoveGameplayEffectCallback);

	// Create the attribute set, this replicates by default
	// Adding it as a subobject of the owning actor of an AbilitySystemComponent
	// automatically registers the AttributeSet with the AbilitySystemComponent
	AttributeSet = CreateDefaultSubobject<UBaseAttributeSet>(TEXT("AttributeSet"));
	AttributeSet->InitHealth(MaxHealth);
	AttributeSet->InitMaxHealth(MaxHealth);
	AttributeSet->InitHealthRegenRate(0.f);

	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(AttributeSet->GetHealthAttribute()).AddUObject(this, &ABattlemageTheEndlessCharacter::HealthChanged);

	ComboManager = CreateDefaultSubobject<UAbilityComboManager>(TEXT("ComboManager"));
	AbilitySystemComponent->AbilityFailedCallbacks.AddUObject(ComboManager, &UAbilityComboManager::OnAbilityFailed);
	ComboManager->AbilitySystemComponent = AbilitySystemComponent;

	ProjectileManager = CreateDefaultSubobject<UProjectileManager>(TEXT("ProjectileManager"));
	ProjectileManager->Initialize(this);
}

void ABattlemageTheEndlessCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->InitAbilityActorInfo(this, this);
		// See details of replication modes https://github.com/tranek/GASDocumentation?tab=readme-ov-file#concepts-asc-rm
		// Update to mixed since this is a player character
		AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);
	}

	// ASC MixedMode replication requires that the ASC Owner's Owner be the Controller.
	SetOwner(NewController);
}

void ABattlemageTheEndlessCharacter::SetupCapsule()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(55.f, 96.0f);
	GetCapsuleComponent()->OnComponentBeginOverlap.AddDynamic(this, &ABattlemageTheEndlessCharacter::OnBaseCapsuleBeginOverlap);
	GetCapsuleComponent()->OnComponentHit.AddDynamic(this, &ABattlemageTheEndlessCharacter::OnProjectileHit);

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

	//Destroy the Player's equipment
	for (auto& slot : Equipment)
	{
		for (auto& pickup : slot.Value.Pickups)
		{
			pickup->Destroy();
		}
	}

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

void ABattlemageTheEndlessCharacter::ToggleAimMode(ETriggerEvent triggerType)
{
	// TODO: Support different zoom levels per spell
	if (triggerType == ETriggerEvent::Triggered)
	{
		IsAiming = true;
	}
	else
	{
		IsAiming = false;
	}

	// Set the timer to adjust the FOV to target if it's not already active
	FTimerManager& timerManager = GetWorld()->GetTimerManager();
	if (!timerManager.IsTimerActive(AimModeTimerHandle))
	{
		FTimerDelegate TimerDelegate = FTimerDelegate::CreateUObject(this, &ABattlemageTheEndlessCharacter::AdjustAimModeFov, 1.f);
		timerManager.SetTimer(AimModeTimerHandle, TimerDelegate, FovUpdateInterval, true);
	}
}

void ABattlemageTheEndlessCharacter::AdjustAimModeFov(float DeltaTime)
{
	float zoomDifference = IsAiming ?  ZoomedFOV - DefaultFOV : DefaultFOV - ZoomedFOV;
	float amountToAdjust = zoomDifference / (TimeToAim / FovUpdateInterval);
 	FirstPersonCamera->SetFieldOfView(FirstPersonCamera->FieldOfView + amountToAdjust);

	if (IsAiming && FirstPersonCamera->FieldOfView <= ZoomedFOV)
	{
		// probably redundant but want to make sure we clamp to exactly the target value
		FirstPersonCamera->SetFieldOfView(ZoomedFOV);
		GetWorld()->GetTimerManager().ClearTimer(AimModeTimerHandle);
	}
	else if (!IsAiming && FirstPersonCamera->FieldOfView >= DefaultFOV)
	{
		// probably redundant but want to make sure we clamp to exactly the target value
		FirstPersonCamera->SetFieldOfView(DefaultFOV);
		GetWorld()->GetTimerManager().ClearTimer(AimModeTimerHandle);
	}
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
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}

		GiveDefaultAbilities();
		GiveStartingEquipment();
	}

	HealthChangedDelegateHandle = AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(AttributeSet->GetHealthAttribute())
		.AddUObject(this, &ABattlemageTheEndlessCharacter::HealthChanged);

	InitHealthbar();
}

void ABattlemageTheEndlessCharacter::InitHealthbar()
{
	if (UHealthBarWidget* healthBarWidget = GetHealthBarWidget())
	{
		healthBarWidget->Init(AbilitySystemComponent, AttributeSet);
	}
}

UHealthBarWidget* ABattlemageTheEndlessCharacter::GetHealthBarWidget()
{
	TArray<USceneComponent*> childComponents;
	GetMesh()->GetChildrenComponents(true, childComponents);
	for (USceneComponent* component : childComponents)
	{
		UWidgetComponent* healthBarComponent = Cast<UWidgetComponent>(component);
		if (!healthBarComponent)
			continue;

		if (UHealthBarWidget* healthBarWidget = Cast<UHealthBarWidget>(healthBarComponent->GetUserWidgetObject()))
		{
			return healthBarWidget;
		}
	}

	return nullptr;
}

void ABattlemageTheEndlessCharacter::GiveStartingEquipment()
{
	// This shouldn't be run if the character already has equipment
	if (Equipment.Num() > 0)
		return;

	// init equipment map
	Equipment.Add(EquipSlot::Primary, FPickups());
	Equipment.Add(EquipSlot::Secondary, FPickups());

	// init equipment abiltiy handles
	if (EquipmentAbilityHandles.Num() == 0)
	{
		EquipmentAbilityHandles.Add(EquipSlot::Primary, FAbilityHandles());
		EquipmentAbilityHandles.Add(EquipSlot::Secondary, FAbilityHandles());
	}

	// add abilities given by Weapons
	for (const TSubclassOf<class APickupActor> PickupType : DefaultEquipment)
	{
		// create the actor
		APickupActor* pickup = GetWorld()->SpawnActor<APickupActor>(PickupType, GetActorLocation(), GetActorRotation());

		if (!pickup || !pickup->Weapon)
			continue;

		// disable collision so we won't block the player
		pickup->BaseCapsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		pickup->BaseCapsule->OnComponentBeginOverlap.RemoveAll(this);

		// Attach the weapon to the appropriate socket
		bool isRightHand = pickup->Weapon->SlotType == EquipSlot::Primary != LeftHanded;
		FName socketName = isRightHand ? FName("GripRight") : FName("GripLeft");
		pickup->Weapon->AttachToComponent(GetMesh(), FAttachmentTransformRules(EAttachmentRule::SnapToTarget, true), socketName);

		// offset the weapon from the socket if needed
		if (pickup->Weapon->AttachmentOffset != FVector::ZeroVector && pickup->Weapon->GetRelativeLocation() == FVector::ZeroVector)
			pickup->Weapon->AddLocalOffset(pickup->Weapon->AttachmentOffset);

		// hide if this isn't the first item (since the first item is equipped by default)
		if (Equipment[pickup->Weapon->SlotType].Pickups.Num() > 0)
		{
			pickup->SetHidden(true);
			pickup->Weapon->SetHiddenInGame(true);
		}

		// add the pickup to the appropriate slot in the equipment map
		Equipment[pickup->Weapon->SlotType].Pickups.Add(pickup);

		// track active spell class if this is a spell, this is used later to switch between spell classes
		if (pickup->Weapon->SlotType == EquipSlot::Secondary && !ActiveSpellClass)
			ActiveSpellClass = pickup;
	}

	// assign abilities and attach the first pickup from each slot
	auto pickups = TArray<TObjectPtr<APickupActor>>();
	if (Equipment[EquipSlot::Primary].Pickups.Num() > 0)
		pickups.Add(Equipment[EquipSlot::Primary].Pickups[0]);
	if (Equipment[EquipSlot::Secondary].Pickups.Num() > 0)
		pickups.Add(Equipment[EquipSlot::Secondary].Pickups[0]);

	for (auto pickup : pickups)
	{
		// Assign the weapon to the appropriate slot
		SetActivePickup(pickup);
	}
}

void ABattlemageTheEndlessCharacter::GiveDefaultAbilities()
{
	if (!AbilitySystemComponent)
	{
		UE_LOG(LogTemplateCharacter, Error, TEXT("'%s' Failed to find an Ability System Component!"), *GetNameSafe(this));
		return;
	}

	// add default abilities
	for (TSubclassOf<UGameplayAbility>& Ability : DefaultAbilities)
	{
		AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(Ability, 1, static_cast<int32>(EGASAbilityInputId::Confirm), this));
	}
}

void ABattlemageTheEndlessCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	TObjectPtr<UBMageCharacterMovementComponent> movement = Cast<UBMageCharacterMovementComponent>(GetCharacterMovement());
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		//EnhancedInputComponent->BindAction(MenuAction, ETriggerEvent::Triggered, this, &ABattlemageTheEndlessCharacter::ToggleMenu);

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

		// Equip spell class actions
		EnhancedInputComponent->BindAction(SpellClassOneAction, ETriggerEvent::Triggered, this, &ABattlemageTheEndlessCharacter::EquipSpellClass, 1);
		EnhancedInputComponent->BindAction(SpellClassTwoAction, ETriggerEvent::Triggered, this, &ABattlemageTheEndlessCharacter::EquipSpellClass, 2);
		EnhancedInputComponent->BindAction(SpellClassThreeAction, ETriggerEvent::Triggered, this, &ABattlemageTheEndlessCharacter::EquipSpellClass, 3);
		EnhancedInputComponent->BindAction(SpellClassFourAction, ETriggerEvent::Triggered, this, &ABattlemageTheEndlessCharacter::EquipSpellClass, 4);

		EnhancedInputComponent->BindAction(NextSpellAction, ETriggerEvent::Triggered, this, &ABattlemageTheEndlessCharacter::SelectActiveSpell, true);
		EnhancedInputComponent->BindAction(PreviousSpellAction, ETriggerEvent::Triggered, this, &ABattlemageTheEndlessCharacter::SelectActiveSpell, false);

		// Casting Related Actions
		EnhancedInputComponent->BindAction(CastingModeAction, ETriggerEvent::Triggered, this, &ABattlemageTheEndlessCharacter::ToggleCastingMode);

		// Aim Mode
		EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Triggered, this, &ABattlemageTheEndlessCharacter::ToggleAimMode, ETriggerEvent::Triggered);
		EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Completed, this, &ABattlemageTheEndlessCharacter::ToggleAimMode, ETriggerEvent::Completed);

		// Bind abilities to input
		if (AbilitySystemComponent) 
		{
			AbilitySystemComponent->BindAbilityActivationToInputComponent(
				EnhancedInputComponent, FGameplayAbilityInputBinds(
					"Confirm", "Cancel", "EGASAbilityInputID", 
					static_cast<int32>(EGASAbilityInputId::Confirm), 
					static_cast<int32>(EGASAbilityInputId::Cancel)));
		}
	}
	else
	{
		UE_LOG(LogTemplateCharacter, Error, TEXT("'%s' Failed to find an Enhanced Input Component!"), *GetNameSafe(this));
	}
}

void ABattlemageTheEndlessCharacter::Crouch() 
{
	// nothing to do if we're already crouching
	if (bIsCrouched)
		return;

	UBMageCharacterMovementComponent* movement = Cast<UBMageCharacterMovementComponent>(GetCharacterMovement());
	if (!movement)
	{
		UE_LOG(LogTemplateCharacter, Error, TEXT("'%s' Failed to find a Movement Component!"), *GetNameSafe(this));
		return;
	}

	ACharacter::Crouch(false);

	if (!movement->IsAbilityActive(MovementAbilityType::Sprint) && !movement->IsAbilityActive(MovementAbilityType::Dodge))
	{
		return;
	}

	// Start a slide if we've made it this far
	movement->TryStartAbility(MovementAbilityType::Slide);
}

void ABattlemageTheEndlessCharacter::TickActor(float DeltaTime, ELevelTick TickType, FActorTickFunction& ThisTickFunction)
{
	// lots of dependencies on movement in here, just get it since we always need it
	UBMageCharacterMovementComponent* movement = Cast<UBMageCharacterMovementComponent>(GetCharacterMovement());

	// Adjust character pitch if sliding
	if (movement->IsAbilityActive(MovementAbilityType::Slide))
	{
		FRotator currentRotation = GetActorRotation();
		currentRotation.Pitch = Cast<USlideAbility>(movement->MovementAbilities[MovementAbilityType::Slide])->LastActualAngle;
		SetActorRotation(currentRotation);
	}

	// Uncrouch if requested unless sliding
	if (movement->ShouldUnCrouch && !movement->IsAbilityActive(MovementAbilityType::Slide))
		DoUnCrouch(movement);

	// call super to apply movement before we evaluate a change in z velocity
	AActor::TickActor(DeltaTime, TickType, ThisTickFunction);
}

void ABattlemageTheEndlessCharacter::DoUnCrouch(UBMageCharacterMovementComponent* movement)
{
	UnCrouch(false);
	movement->ShouldUnCrouch = false;

	// get movement component as BMageCharacterMovementComponent
	UBMageCharacterMovementComponent* mageMovement = Cast<UBMageCharacterMovementComponent>(movement);
	// end slide if we're sliding
	mageMovement->TryEndAbility(MovementAbilityType::Slide);
}

void ABattlemageTheEndlessCharacter::EndSlide(UCharacterMovementComponent* movement)
{
	// Reset pitch if needed
	FRotator currentRotation = GetActorRotation();
	if (currentRotation.Pitch > 0.0001f)
		SetActorRotation(FRotator(0.f, currentRotation.Yaw, currentRotation.Roll));

	if(UBMageCharacterMovementComponent* mageMovement = Cast<UBMageCharacterMovementComponent>(movement))
		mageMovement->TryEndAbility(MovementAbilityType::Slide);
}

void ABattlemageTheEndlessCharacter::StartSprint()
{
	TObjectPtr<UBMageCharacterMovementComponent> movement = Cast<UBMageCharacterMovementComponent>(GetCharacterMovement());
	if (!movement)
		return;

	movement->TryStartAbility(MovementAbilityType::Sprint);
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

void ABattlemageTheEndlessCharacter::RequestUnCrouch()
{
	if (UBMageCharacterMovementComponent* mageMovement = Cast<UBMageCharacterMovementComponent>(GetCharacterMovement()))
		mageMovement->RequestUnCrouch();
}

FVector ABattlemageTheEndlessCharacter::CurrentGripOffset(FName SocketName)
{
	return GetWeapon(LeftHanded ? EquipSlot::Primary : EquipSlot::Secondary)->Weapon->MuzzleOffset;
}

void ABattlemageTheEndlessCharacter::EquipSpellClass(int slotNumber)
{
	// invalid state
	if (slotNumber > Equipment[EquipSlot::Secondary].Pickups.Num())
	{
		UE_LOG(LogExec, Error, TEXT("'%s' Attempted to equip a spell class that doesn't exist!"), *GetNameSafe(this));
		return;
	}

	ActiveSpellClass = Equipment[EquipSlot::Secondary].Pickups[slotNumber - 1];
	SetActivePickup(ActiveSpellClass);
}

bool ABattlemageTheEndlessCharacter::TryRemovePreviousAbilityEffect(AGameplayCueNotify_Actor* Notify)
{
	if (!AbilitySystemComponent)
		return false;

	// identify which effect spawned the provided cue
	//auto EffectTag = Notify->DeriveGameplayCueTagFromAssetName();

	//// Get active gameplay effects
	//TArray<FActiveGameplayEffectHandle> ActiveEffects = AbilitySystemComponent->GetActiveEffects(FGameplayEffectQuery());

	//// remove the ability effect
	//AbilitySystemComponent->RemoveActiveGameplayEffect(ActiveEffects[0]);

	return true;
}

void ABattlemageTheEndlessCharacter::PawnClientRestart()
{
	// Add Input Mapping Context and equipment, only do this for controlled players
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		GiveDefaultAbilities();
		GiveStartingEquipment();
	}

	// turns out if you forget to call this, your input doesn't get set up
	Super::PawnClientRestart();
}

void ABattlemageTheEndlessCharacter::Move(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller == nullptr)
		return;

	TObjectPtr<UBMageCharacterMovementComponent> movement = Cast<UBMageCharacterMovementComponent>(GetCharacterMovement());
	if (!movement)
		return;

	// add movement, only apply forward if we're wall running
	if (movement->IsAbilityActive(MovementAbilityType::WallRun))
	{
		AddMovementInput(GetActorForwardVector(), MovementVector.Y);
	}
	else if (movement->IsAbilityActive(MovementAbilityType::Slide))
	{
		// slide ignores lateral input and reduces impact of backward input
		AddMovementInput(GetActorForwardVector(), MovementVector.Y * MovementVector.Y < 0.f ? SlideBrakingFactor : 1.0f);
	}
	else
	{
		AddMovementInput(GetActorForwardVector(), MovementVector.Y);
		AddMovementInput(GetActorRightVector(), MovementVector.X);
	}

	movement->ApplyInput();

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
	else if (bIsCrouched)
		UnCrouch();

	bool wallrunEnded = mageMovement->TryEndAbility(MovementAbilityType::WallRun);

	// movement redirection logic
	if (JumpCurrentCount < JumpMaxCount)
		RedirectVelocityToLookDirection(wallrunEnded);

	Super::Jump();
}

// TODO: Refactor this to movement component
void ABattlemageTheEndlessCharacter::RedirectVelocityToLookDirection(bool wallrunEnded)
{
	UBMageCharacterMovementComponent* movement = Cast<UBMageCharacterMovementComponent>(GetCharacterMovement());
	if (!movement || movement->MovementMode != EMovementMode::MOVE_Falling)
		return;

	// jumping past apex will have the different gravity scale, reset it to the base
	if (movement->GravityScale != movement->CharacterBaseGravityScale)
		movement->GravityScale = movement->CharacterBaseGravityScale;
	
	// use the already normalized rotation vector as the base
	// NOTE: we are using the camera's rotation instead of the character's rotation to support wall running, which disables camera based character yaw control
	FVector targetDirectionVector = GetActiveCamera()->GetComponentRotation().Vector();
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

void ABattlemageTheEndlessCharacter::SetActivePickup(APickupActor* pickup)
{
	if (!pickup || !pickup->Weapon)
		return;

	bool isRightHand = pickup->Weapon->SlotType == EquipSlot::Primary != LeftHanded;

	APickupActor* currentItem = isRightHand ? RightHandWeapon : LeftHandWeapon;
	UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(Controller->InputComponent);

	// if there is an equipped item in the requested slot, unequip it and remove abilities it grants
	if (currentItem)
	{
		// Hide the weapon being removed from active status
		currentItem->SetHidden(true);
		currentItem->Weapon->SetHiddenInGame(true);

		// remove its mapping context if applicable
		if (currentItem->Weapon->WeaponMappingContext)
		{
			currentItem->Weapon->RemoveContext(this);
		}

		// clear abilities granted by current weapon
		for (FGameplayAbilitySpecHandle Ability : EquipmentAbilityHandles[currentItem->Weapon->SlotType].Handles)
		{
			AbilitySystemComponent->ClearAbility(Ability);
		}

		// remove bindings for the current weapon
		for(auto handle: EquipmentBindingHandles[currentItem].Handles)
		{
			EnhancedInputComponent->RemoveBindingByHandle(handle);
		}

		EquipmentBindingHandles.Remove(currentItem);
		EquipmentAbilityHandles[currentItem->Weapon->SlotType].Handles.Empty();
	}

	// unhide newly activated weapon
	pickup->SetHidden(false);
	pickup->Weapon->SetHiddenInGame(false);

	// set it to the appropriate hand
	isRightHand ? RightHandWeapon = pickup : LeftHandWeapon = pickup;

	// The handles in GAS change each time we grant an ability, so we need to reset them each time we equip a new weapon
	// TODO: We should be able to assign all abilities on begin play and not have to do this since we're explicit when activating an ability
	if (ComboManager->Combos.Contains(pickup))
		ComboManager->Combos.Remove(pickup);

	// grant abilities for the new weapon
	// Needs to happen before bindings since bindings look up assigned abilities
	for (TSubclassOf<UGameplayAbility>& ability : pickup->Weapon->GrantedAbilities)
	{
		FGameplayAbilitySpecHandle handle = AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(ability, 1, static_cast<int32>(EGASAbilityInputId::Confirm), this));
		ComboManager->AddAbilityToCombo(pickup, ability->GetDefaultObject<UAttackBaseGameplayAbility>(), handle);
	}
	// to avoid nullptr
	if (pickup->Weapon->GrantedAbilities.Num() > 0)
	{
		pickup->Weapon->SelectedAbility = pickup->Weapon->GrantedAbilities[0];
	}

	// Set the active spell to the first granted ability if it is not already set and the item is a spell focus
	if (pickup->Weapon->SlotType == EquipSlot::Secondary && pickup->Weapon->GrantedAbilities.Num() > 0 && !pickup->Weapon->SelectedAbility)
	{
		pickup->Weapon->SelectedAbility = pickup->Weapon->GrantedAbilities[0];
	}

	// add the mapping context and bindings if applicable
	if (pickup->Weapon->WeaponMappingContext)
	{
		APlayerController* PlayerController = Cast<APlayerController>(Controller);
		UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer());
		if (PlayerController && Subsystem)
		{
			Subsystem->AddMappingContext(pickup->Weapon->WeaponMappingContext, 0);
		}
	}	

	// track key bindings for the new weapon
	if (EnhancedInputComponent)
	{
		auto bindingHandles = FBindingHandles();
		for (ETriggerEvent triggerEvent : TEnumRange<ETriggerEvent>())
		{
			bindingHandles.Handles.Add(
				EnhancedInputComponent->BindAction(
					pickup->Weapon->FireActionTap, 
					triggerEvent,
					this, 
					pickup->PickupType == EPickupType::Weapon ? &ABattlemageTheEndlessCharacter::ProcessMeleeInput : & ABattlemageTheEndlessCharacter::ProcessSpellInput,
					pickup, 
					EAttackType::Light, 
					triggerEvent)
				.GetHandle());

			// Charge abilities start immediately 
			bindingHandles.Handles.Add(
				EnhancedInputComponent->BindAction(
					pickup->Weapon->FireActionHold,
					triggerEvent,
					this,
					pickup->PickupType == EPickupType::Weapon ? &ABattlemageTheEndlessCharacter::ProcessMeleeInput : &ABattlemageTheEndlessCharacter::ProcessSpellInput_Charged,
					pickup,
					EAttackType::Heavy,
					triggerEvent)
				.GetHandle());

			// placed abilities
			bindingHandles.Handles.Add(
				EnhancedInputComponent->BindAction(
					pickup->Weapon->FireActionHoldAndRelease,
					triggerEvent,
					this,
					pickup->PickupType == EPickupType::Weapon ? &ABattlemageTheEndlessCharacter::ProcessMeleeInput : &ABattlemageTheEndlessCharacter::ProcessSpellInput_Placed,
					pickup,
					EAttackType::Light,
					triggerEvent)
				.GetHandle());
		}
		EquipmentBindingHandles.Add(pickup, bindingHandles);
	}
	// todo: make this work
	// Grant abilities, but only on the server	
	/*if (Role != ROLE_Authority || !AbilitySystemComponent.IsValid() || AbilitySystemComponent->bCharacterAbilitiesGiven)
	{
		return;
	}*/
}

void ABattlemageTheEndlessCharacter::SelectActiveSpell(bool nextOrPrevious)
{
	if (!ActiveSpellClass || !ActiveSpellClass->Weapon)
		return;

	ActiveSpellClass->Weapon->NextOrPreviousSpell(nextOrPrevious);
}

APickupActor* ABattlemageTheEndlessCharacter::GetWeapon(EquipSlot SlotType)
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

void ABattlemageTheEndlessCharacter::OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode)
{
	if (PrevMovementMode == EMovementMode::MOVE_Falling && GetCharacterMovement()->MovementMode == EMovementMode::MOVE_Walking)
	{
		// TODO: Use anim notify instead
		UGameplayStatics::PlaySoundAtLocation(this,
			JumpLandingSound,
			GetActorLocation(), 1.0f);

		UBMageCharacterMovementComponent* movement = Cast<UBMageCharacterMovementComponent>(GetCharacterMovement());
		if (movement->bWantsToCrouch && movement->IsAbilityActive(MovementAbilityType::Sprint))
		{
			movement->TryStartAbility(MovementAbilityType::Slide);
		}
		else
			movement->ResetWalkSpeed();
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

void ABattlemageTheEndlessCharacter::ToggleCastingMode()
{
	IsInCastingMode = !IsInCastingMode;
	UAnimInstance* anim = GetMesh()->GetAnimInstance();
	if (!LeftHandWeapon || !anim)
		return;

	if (IsInCastingMode)
	{
		// play the casting mode animation
		anim->Montage_Play(CastingModeMontage, 1.f);
	}
	else
	{
		// stop the casting mode animation
		anim->Montage_Stop(0.2f);
	}
}

// TODO: Deprecate this
bool ABattlemageTheEndlessCharacter::IsDodging()
{
	UBMageCharacterMovementComponent* movement = Cast<UBMageCharacterMovementComponent>(GetCharacterMovement());
	return movement && movement->IsAbilityActive(MovementAbilityType::Dodge);
}

FName ABattlemageTheEndlessCharacter::GetTargetSocketName(EquipSlot SlotType)
{
	if (SlotType == EquipSlot::Primary)
		return FName(LeftHanded ? "GripLeft" : "GripRight");
	else
		return FName(LeftHanded ? "GripRight" : "GripLeft");
}

void ABattlemageTheEndlessCharacter::OnBaseCapsuleBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// if the other actor is not a CheckPoint, return
	if (ACheckPoint* checkpoint = Cast<ACheckPoint>(OtherActor))
		LastCheckPoint = checkpoint;
}

void ABattlemageTheEndlessCharacter::HealthChanged(const FOnAttributeChangeData& Data)
{
	// If health isn't set up, exit
	if (AttributeSet->GetMaxHealth() == 0)
		return;
}

void ABattlemageTheEndlessCharacter::ProcessMeleeInput(APickupActor* PickupActor, EAttackType AttackType, ETriggerEvent triggerEvent)
{
	switch (triggerEvent)
	{
		// melee requires a completed event whether that's tap for light or hold for heavy
		case ETriggerEvent::Completed:
		{
			// if we have a pickup actor, process the input
			if (PickupActor)
			{
				//ProcessSpellInput(PickupActor, AttackType, triggerEvent);
				ProcessInputAndBindAbilityCancelled(PickupActor, AttackType, triggerEvent);
			}
			else
			{
				UE_LOG(LogTemplateCharacter, Warning, TEXT("'%s' ProcessMeleeInput called without a PickupActor!"), *GetNameSafe(this));
			}
			break;
		}
		default:
			break;
	}

}

void ABattlemageTheEndlessCharacter::ProcessSpellInput(APickupActor* PickupActor, EAttackType AttackType, ETriggerEvent triggerEvent)
{
	if (triggerEvent != ETriggerEvent::Triggered)
	{
		// This handler is for tap spells, so we only handle triggered events
		return;
	}

	if (!PickupActor)
	{
		UE_LOG(LogTemplateCharacter, Warning, TEXT("'%s' ProcessSpellInput called without a PickupActor, how the heck did you manage that?"), *GetNameSafe(this));
		return;	
	}

	auto selectedAbilitySpec = AbilitySystemComponent->FindAbilitySpecFromClass(PickupActor->Weapon->SelectedAbility);
	auto ability = Cast<UAttackBaseGameplayAbility>(selectedAbilitySpec->Ability);
	auto isComboActive = ComboManager->Combos.Contains(PickupActor) && ComboManager->Combos[PickupActor].ActiveCombo;

	if (ability && (ability->ChargeDuration > 0.001f || (ability->HitType == HitType::Placed && !isComboActive)))
	{
		// if the ability is charged or placed and we aren't in a combo, this is the wrong handler
		return;
	}

	if (ability->HitType == HitType::Actor)
	{
		SpawnSpellActors(ability, false);
	}

	GEngine->AddOnScreenDebugMessage(-1, 1.50f, FColor::Blue, FString::Printf(TEXT("ProcessSpellInput: Ability %s"),
		*(ability->GetAbilityName().ToString())));

	ProcessInputAndBindAbilityCancelled(PickupActor, AttackType, triggerEvent);
}

void ABattlemageTheEndlessCharacter::ProcessSpellInput_Charged(APickupActor* PickupActor, EAttackType AttackType, ETriggerEvent triggerEvent)
{
	if (triggerEvent != ETriggerEvent::Started && triggerEvent != ETriggerEvent::Completed)
	{
		// we only handle started and completed events for charged abilities
		return;
	}

	auto selectedAbilitySpec = AbilitySystemComponent->FindAbilitySpecFromClass(ActiveSpellClass->Weapon->SelectedAbility);
	auto ability = Cast<UAttackBaseGameplayAbility>(selectedAbilitySpec->Ability);

	GEngine->AddOnScreenDebugMessage(-1, 1.50f, FColor::Blue, FString::Printf(TEXT("ProcessSpellInput_Charged: Ability %s"),
		*(ability->GetAbilityName().ToString())));

	if (!ability || ability->ChargeDuration <= 0.0001f)
	{
		// if the ability is not charged, this is the wrong handler
		return;
	}

	auto activeInstances = Cast<UGA_WithEffectsBase>(selectedAbilitySpec->Ability)->GetAbilityActiveInstances(selectedAbilitySpec);
	TObjectPtr<UGA_WithEffectsBase> activeInstance = activeInstances.Num() > 0 ? activeInstances[0] : nullptr;
	auto attackAbility = Cast<UAttackBaseGameplayAbility>(activeInstance);
	if (!attackAbility)
		return;

	switch (triggerEvent)
	{
	case ETriggerEvent::Started:
	{
		bool success = AbilitySystemComponent->TryActivateAbility(selectedAbilitySpec->Handle, true);
		if (!success && GEngine)
			GEngine->AddOnScreenDebugMessage(-1, 1.5f, FColor::Red, FString::Printf(TEXT("Ability %s failed to activate"), *selectedAbilitySpec->Ability->GetName()));

		FTimerDelegate TimerDelegate = FTimerDelegate::CreateUObject(this, &ABattlemageTheEndlessCharacter::ChargeSpell, attackAbility);
		GetWorld()->GetTimerManager().SetTimer(ChargeSpellTimerHandle, TimerDelegate, 1.0f / (float)TickRate, true);

		if (GEngine)
			GEngine->AddOnScreenDebugMessage(-1, 1.5f, FColor::Yellow, FString::Printf(TEXT("Ability %s is charging"), *selectedAbilitySpec->Ability->GetName()));
		break;
	}
	case ETriggerEvent::Completed:
	{
		// if charge spell and completed event, clear the charge timer
		GetWorld()->GetTimerManager().ClearTimer(ChargeSpellTimerHandle);
		PostAbilityActivation(attackAbility);
		// This is a hack to call EndAbility without needing to spoof up all the params
		attackAbility->OnMontageCompleted();
		break;
	}
	// We don't need to do anything for ongoing since we've got the repeating timer handling it on a set interval
	default:
		break;
	}
}

void ABattlemageTheEndlessCharacter::ProcessSpellInput_Placed(APickupActor* PickupActor, EAttackType AttackType, ETriggerEvent triggerEvent)
{
	auto selectedAbilitySpec = AbilitySystemComponent->FindAbilitySpecFromClass(ActiveSpellClass->Weapon->SelectedAbility);
	auto ability = Cast<UAttackBaseGameplayAbility>(selectedAbilitySpec->Ability);

	if (!ability || ability->HitType != HitType::Placed)
	{
		// if the ability is not placed, this is the wrong handler
		return;
	}

	_secondsSinceLastPrint += GetWorld()->GetTime().GetDeltaWorldTimeSeconds();
	if (_secondsSinceLastPrint > 0.33f)
	{
		GEngine->AddOnScreenDebugMessage(-1, 1.50f, FColor::Blue, FString::Printf(TEXT("ProcessSpellInput_Placed: Ability %s"),
			*(ability->GetAbilityName().ToString())));

		_secondsSinceLastPrint = 0;
	}

	// check if this ability has a combo and we are past the first part
	if (ComboManager->Combos.Contains(PickupActor) && ComboManager->Combos[PickupActor].ActiveCombo)
	{
		// if we are in a combo, we should not be placing spells
		return;
	}

	auto movement = GetCharacterMovement();

	// Placed spells have the following logic flow based on triggerEvent:
	// Started: Display the "ghost" spell
	// Ongoing: Update the "ghost" spell location based on player aim and max range
	// Completed: Spawn the spell at the "ghost" location and end the ability
	switch (triggerEvent)
	{
		// The button was pressed for the first time
		case ETriggerEvent::Started:
		{
			SpawnSpellActors(ability, true);
			break;
		}
		// The button is being held
		case ETriggerEvent::Ongoing:
		{
			for (auto ghostActor : ability->GetPlacementGhosts())
			{
				if (ghostActor && IsValid(ghostActor))
				{
					PositionSpellActor(ability, ghostActor);
				}
			}
			break;
		}
		// Casting was cancelled
		case ETriggerEvent::Canceled:
		{
			// destroy all placement ghosts
			for (auto ghostActor : ability->GetPlacementGhosts())
				ghostActor->Destroy();
			break;
		}
		// Casting was confirmed by releasing button
		case ETriggerEvent::Triggered:
		{			
			ComboManager->ProcessInput(PickupActor, AttackType);
			// reset materials on all placement ghosts
			for (auto ghostActor : ability->GetPlacementGhosts())
			{
				if (!ghostActor || !IsValid(ghostActor))
					continue;

				// re-enable collision
				ghostActor->SetActorEnableCollision(true);

				// account for time spent placing the spell if it has a lifespan
				if (ghostActor->InitialLifeSpan > 0.f)
					ghostActor->SetLifeSpan(ghostActor->InitialLifeSpan);

				for (UActorComponent* component : ghostActor->GetComponents())
				{
					auto meshComponent = Cast<UStaticMeshComponent>(component);
					if (meshComponent)
					{
						// This design assumes the standard material is the parent of the ghost material
						if (ghostActor->PlacementGhostMaterial && ghostActor->PlacementGhostMaterial->Parent)
							meshComponent->SetMaterial(0, ghostActor->PlacementGhostMaterial->Parent);
						continue;
					}

					auto niagaraComponent = Cast<UNiagaraComponent>(component);
					if (niagaraComponent)
					{
						niagaraComponent->ActivateSystem();
						continue;
					}
				}
			}

			// stop tracking placement ghosts after making them real
			ability->ClearPlacementGhosts();

			break;
		}
		default:
			break;
	}
}

void ABattlemageTheEndlessCharacter::SpawnSpellActors(UAttackBaseGameplayAbility* ability, bool isGhost)
{
	auto defaultLocation = FVector(0, 0, -9999);
	// TODO: Add an animation for placing a spell, can probably reuse the charging animation?

	// A Placed spell will produce one or more hit effect actors at the target location, so we need to ghost all of them
	// to do that, we need to create actor(s) at the target location and store them 
	for (auto hitEffectActor : ability->HitEffectActors)
	{
		// spawn the actor way below the world and then reposition it (we need the instance to calculate dimensions)
		auto spellActor = GetWorld()->SpawnActor<AHitEffectActor>(hitEffectActor, defaultLocation, GetCharacterMovement()->GetLastUpdateRotation());
		// required for calculations of positional differences between instigator and target
		spellActor->Instigator = this;
		if (spellActor && IsValid(spellActor))
		{
			spellActor->SetOwner(this);

			if (isGhost)
				spellActor->SetActorEnableCollision(false);

			PositionSpellActor(ability, spellActor);

			// for each Static Mesh Component on this actor, check if there is an instance of the material with Translucent blend mode
			for (UActorComponent* component : spellActor->GetComponents())
			{
				// check if the component is a mesh component
				auto meshComponent = Cast<UStaticMeshComponent>(component);
				if (!meshComponent)
					continue;

				if (isGhost)
				{
					auto material = meshComponent->GetMaterial(0);
					meshComponent->SetMaterial(0, spellActor->PlacementGhostMaterial);
				}
			}

			if (isGhost)
				ability->RegisterPlacementGhost(spellActor);
		}
	}
}

void ABattlemageTheEndlessCharacter::PositionSpellActor(UAttackBaseGameplayAbility* ability, AHitEffectActor* hitEffectActor)
{
	// perform a ray cast up to max spell range to find a valid spawn location
	auto ignoreActors = TArray<AActor*>();
	ignoreActors.Add(hitEffectActor);

	auto CastHit = Traces::LineTraceFromCharacter(this, GetMesh(), FName("cameraSocket"), GetActiveCamera()->GetComponentRotation(), ability->MaxRange, ignoreActors);
	auto hitComponent = CastHit.GetComponent();
	FVector spawnLocation;

	// if we hit a character, place the effect just shy of them and SnapActorToGround will handle the rest
	auto isDirectHit = CastHit.GetActor() && CastHit.GetActor()->IsA<ACharacter>();
	if (isDirectHit)
	{
		auto castNormal = CastHit.Location - GetMesh()->GetSocketLocation(FName("cameraSocket"));
		castNormal.Normalize();
		spawnLocation = CastHit.Location - castNormal * 30.f;
	}
	else
		spawnLocation = hitComponent ? CastHit.Location : CastHit.TraceEnd;

	// move the spawn location up by half the height of the hit effect actor if we are placing on the ground
	//if (hitComponent && hitComponent->IsA<UPrimitiveComponent>())
	//{
	//	// we need to consider all components since we have collision disabled at this point
	//	//	this can be a performance issue if actors get too complex
	//	auto bounding = hitEffectActor->GetComponentsBoundingBox(true);
	//	spawnLocation.Z += (bounding.Max.Z - bounding.Min.Z) / 2.f;
	//}

	hitEffectActor->SetActorLocation(spawnLocation);

	auto rotation = GetCharacterMovement()->GetLastUpdateRotation();
	// Use look rotation if actor isn't meant to snap to ground
	if (!hitEffectActor->SnapToGround)
		rotation.Pitch = GetActiveCamera()->GetComponentRotation().Pitch;

	hitEffectActor->SetActorRotation(rotation);
	hitEffectActor->SnapActorToGround(isDirectHit ? FHitResult() : CastHit);

	// handle niagara user parameters which need to rotate with character
	//hitEffectActor->GetComponentByClass<UNiagaraComponent>()->SetWorldRotation(rotation);
}

void ABattlemageTheEndlessCharacter::ProcessInputAndBindAbilityCancelled(APickupActor* PickupActor, EAttackType AttackType, ETriggerEvent triggerEvent)
{
	// Let combo manager resolve and dispatch ability as needed
	auto specHandle = ComboManager->ProcessInput(PickupActor, AttackType);
	if (!specHandle.IsValid())
		return;

	// find the ability instance
	auto instances = Cast<UAttackBaseGameplayAbility>(AbilitySystemComponent->FindAbilitySpecFromHandle(specHandle)->Ability)
		->GetAbilityActiveInstances(AbilitySystemComponent->FindAbilitySpecFromHandle(specHandle));

	if (instances.Num() == 0)
		return;

	if (auto attackAbility = Cast<UAttackBaseGameplayAbility>(instances[0]))
		PostAbilityActivation(attackAbility);
}

void ABattlemageTheEndlessCharacter::PostAbilityActivation(UAttackBaseGameplayAbility* ability)
{
	// If the ability has projectiles to spawn, spawn them and exit
	//	Currently an ability can apply effects either on hit or to self, but not both
	if (ability->HitType == HitType::Projectile && ability->ProjectileConfiguration.ProjectileClass)
	{
		HandleProjectileSpawn(ability);

		// for charge spells we need to end the ability after the hit check
		if (ability->ChargeDuration > 0.001f)
			ability->EndSelf();

		return;
	}
	else if (ability->HitType == HitType::HitScan)
	{
		HandleHitScan(ability);

		// for charge spells we need to end the ability after the hit check
		if (ability->ChargeDuration > 0.001f)
			ability->EndSelf();

		return;
	}

	// If there are no effects, we have no need for a cancel callback
	if (ability->EffectsToApply.Num() == 0)
		return;

	// Otherwise bind the cancel event to remove the active effects immediately
	AbilitySystemComponent->OnAbilityEnded.AddUObject(this, &ABattlemageTheEndlessCharacter::OnAbilityCancelled);
}

// TODO: Why is this in character instead of the ability?
void ABattlemageTheEndlessCharacter::HandleProjectileSpawn(UAttackBaseGameplayAbility* ability)
{
	// I'm unclear on why, but the address of the character seems to change at times
	if (ProjectileManager->OwnerCharacter != this)
		ProjectileManager->OwnerCharacter = this;

	TArray<ABattlemageTheEndlessProjectile*> projectiles;

	// if the projectiles are spawned from an actor, use that entry point
	if (ability->ProjectileConfiguration.SpawnLocation == FSpawnLocation::Player)
	{
		projectiles = ProjectileManager->SpawnProjectiles_Actor(ability, ability->ProjectileConfiguration, this);
	}
	// We can only spawn at last ability location if we have a niagara instance
	//  TODO: support spawning projectiles based on a previous ability's actor(s) as well
	else if (ability->ProjectileConfiguration.SpawnLocation == FSpawnLocation::SpellFocus)
	{
		const FRotator spawnRotation = Cast<APlayerController>(Controller)->PlayerCameraManager->GetCameraRotation();
		auto socketName = LeftHanded ? FName("gripRight") : FName("gripLeft");
		auto spawnLocation = GetMesh()->GetSocketLocation(socketName) + spawnRotation.RotateVector(CurrentGripOffset(socketName));

		// We are making the potentially dangerous assumption that there is only 1 instance
		projectiles = ProjectileManager->SpawnProjectiles_Location(ability, ability->ProjectileConfiguration,
			spawnRotation, spawnLocation, FVector::OneVector, ActiveSpellClass);
	}
	else if (ability->ProjectileConfiguration.SpawnLocation == FSpawnLocation::PreviousAbility)
	{
		if (ComboManager->LastAbilityNiagaraInstance)
		{
			// We are making the potentially dangerous assumption that there is only 1 instance
			projectiles = ProjectileManager->SpawnProjectiles_Location(
				ability, ability->ProjectileConfiguration, ComboManager->LastAbilityNiagaraInstance->GetComponentRotation(),
				ComboManager->LastAbilityNiagaraInstance->GetComponentLocation());
		}
	}
	else
	{
		UE_LOG(LogExec, Error, TEXT("'%s' Attempted to spawn projectiles from an invalid location!"), *GetNameSafe(this));
	}

	for (auto projectile : projectiles)
	{
		GetCapsuleComponent()->IgnoreActorWhenMoving(projectile, true);
		projectile->GetCollisionComp()->OnComponentHit.AddDynamic(ability, &UAttackBaseGameplayAbility::OnHit);
	}

}

void ABattlemageTheEndlessCharacter::HandleHitScan(UAttackBaseGameplayAbility* ability)
{
	// perform a line trace to determine if the ability hits anything
	// Uses camera socket as the start location
	FVector startLocation = GetMesh()->GetSocketLocation(FName("cameraSocket"));

	// Figure out the end location based on the ability's max range and current look direction
	UCharacterMovementComponent* movement = GetCharacterMovement();
	auto rotation = movement->GetLastUpdateRotation();

	// always use first person camera for the pitch since it's tied to the character's look direction
	rotation.Pitch = FirstPersonCamera->GetComponentRotation().Pitch;
	FVector endLocation = startLocation + (rotation.Vector() * ability->MaxRange);

	//DrawDebugLine(GetWorld(), startLocation, endLocation, FColor::Red, false, 5.0f, 0, 1.0f);

	auto params = FCollisionQueryParams(FName(TEXT("LineTrace")), true, this);
	FHitResult hit = Traces::LineTraceGeneric(GetWorld(), params, startLocation, endLocation);

	auto hitActor = hit.GetActor();
	if (!hitActor)
		return;

	ABattlemageTheEndlessCharacter* hitCharacter = Cast<ABattlemageTheEndlessCharacter>(hit.GetActor());
	// if we hit something other than a character and this is a chain attack, render the hit at that location
	if (hitActor && !hitCharacter && ability->ChainSystem)
	{
		// We don't need to keep the reference, UE will handle disposing when it is destroyed
		auto chainEffectActor = GetWorld()->SpawnActor<AHitScanChainEffect>(AHitScanChainEffect::StaticClass(), GetActorLocation(), FRotator::ZeroRotator);
		chainEffectActor->Init(this, hitActor, ability->ChainSystem, hit.Location);
		return;
	}

	// if we hit a character apply effects to them (and chain if needed)
	TArray<ABattlemageTheEndlessCharacter*> hitCharacters = { hitCharacter };
	if (ability->NumberOfChains > 0)
	{
		hitCharacters.Append(GetChainTargets(ability->NumberOfChains, ability->ChainDistance, hitCharacter));
	}

	if (ability->EffectsToApply.Num() == 0)
		return;
		
	// Handle chain delay if needed
	// floating point precision, woo
	if (FMath::Abs(ability->ChainDelay) < 0.00001f)
	{
		if (hitCharacters.Num() != 0)
		{
			for (auto applyTo : hitCharacters)
				ability->ApplyEffects(applyTo, applyTo->AbilitySystemComponent, this);
		}
	}
	else
	{
		for (int i = 0; i < hitCharacters.Num(); ++i)
		{
			// No delay for the first target
			if (i == 0)
			{
				ability->ApplyEffects(hitCharacters[i], hitCharacters[i]->AbilitySystemComponent, this);
				continue;
			}

			// NOTE: Apparently this binding is not smart enough to handle default params, automatic casting of nullptrs, or downcasting so we need to do all of those manually
			FTimerDelegate TimerDelegate = FTimerDelegate::CreateUObject(
				ability, 
				&UAttackBaseGameplayAbility::ApplyChainEffects, 
				(AActor*)hitCharacters[i], 
				(UAbilitySystemComponent*)hitCharacters[i]->AbilitySystemComponent, 
				(AActor*)hitCharacters[i-1],
				(AActor*)nullptr,
				i == ability->NumberOfChains
			);

			GetWorld()->GetTimerManager().SetTimer(ability->GetChainTimerHandles()[i - 1], TimerDelegate, ability->ChainDelay * i, false);
		}
	}
}

TArray<ABattlemageTheEndlessCharacter*> ABattlemageTheEndlessCharacter::GetChainTargets(int NumberOfChains, float ChainDistance, ABattlemageTheEndlessCharacter* HitActor)
{
	auto allBMages = TArray<AActor*>();
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ABattlemageTheEndlessCharacter::StaticClass(), allBMages);
	
	auto theoreticalMaxDistance = NumberOfChains * ChainDistance;

	// Filter down to only the BMages within the theoreticalMaxDistance
	allBMages = allBMages.FilterByPredicate(
		[&HitActor, theoreticalMaxDistance, this](const AActor* a) {
			return a != this && a->GetDistanceTo(HitActor) <= theoreticalMaxDistance;
		});

	// If there's nothing to chain to, return an empty array
	if (allBMages.Num() == 0)
		return TArray<ABattlemageTheEndlessCharacter*>();

	int remainingChains = NumberOfChains;
	auto chainTargets = TArray<ABattlemageTheEndlessCharacter*>();
	ABattlemageTheEndlessCharacter* nextChainTarget = HitActor;
	while (remainingChains > 0)
	{
		nextChainTarget = GetNextChainTarget(ChainDistance, nextChainTarget, allBMages);
		// If we couldn't find another eligible target, break
		if (!nextChainTarget)
			break;

		chainTargets.Add(nextChainTarget);

		--remainingChains;
	}

	return chainTargets;
}

ABattlemageTheEndlessCharacter* ABattlemageTheEndlessCharacter::GetNextChainTarget(float ChainDistance, AActor* ChainActor, TArray<AActor*> Candidates)
{
	// If it's 0 we shouldn't have gotten here, if it's 1 then only ChainActor was passed
	if (Candidates.Num() <= 1)
		return nullptr;

	Candidates.Sort([ChainActor](const AActor& a, const AActor& b) {
		return a.GetDistanceTo(ChainActor) < b.GetDistanceTo(ChainActor);
	});

	for (auto actor : Candidates)
	{
		if (actor == ChainActor)
			continue;

		// check for line of sight
		// TODO: Use a specific trace channel rather than all of them
		auto params = FCollisionQueryParams(FName(TEXT("LineTrace")), true, ChainActor);
		auto hitResult = Traces::LineTraceGeneric(GetWorld(), params, ChainActor->GetActorLocation(), actor->GetActorLocation());
		auto hitActor = hitResult.GetActor();
		if (!hitActor || hitActor != actor)
			continue;

		// If we've passed the checks, this is the best candidate
		return Cast<ABattlemageTheEndlessCharacter>(actor);
	}

	// we will only hit this if none of the candidates are in line of sight
	return nullptr;
}

void ABattlemageTheEndlessCharacter::OnAbilityCancelled(const FAbilityEndedData& endData)
{
	if (!endData.bWasCancelled)
		return;

	auto ability = Cast<UAttackBaseGameplayAbility>(AbilitySystemComponent->FindAbilitySpecFromHandle(endData.AbilitySpecHandle)->Ability);
	for (TSubclassOf<UGameplayEffect> effect : ability->EffectsToApply)
	{
		if (effect->GetDefaultObject<UGameplayEffect>()->DurationPolicy != EGameplayEffectDurationType::HasDuration)
			continue;

		AbilitySystemComponent->RemoveActiveGameplayEffectBySourceEffect(effect, AbilitySystemComponent, 1);
	}
}

void ABattlemageTheEndlessCharacter::OnProjectileHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	ABattlemageTheEndlessProjectile* projectile = Cast<ABattlemageTheEndlessProjectile>(OtherActor);
	if (!projectile)
		return;
	
	UAttackBaseGameplayAbility* ability = Cast<UAttackBaseGameplayAbility>(projectile->SpawningAbility);
	if (!ability)
		return;

	// apply effects to the hit actor (this character)
	// TODO: maybe package instigator with the ability so we can trace back to the source of the ability
	ability->ApplyEffects(this, AbilitySystemComponent, nullptr, projectile);
	if (projectile->ShouldDestroyOnHit())
		projectile->Destroy();
}

void ABattlemageTheEndlessCharacter::ChargeSpell(UAttackBaseGameplayAbility* ability)
{
	bool isChargedBefore = ability->IsCharged();
	ability->HandleChargeProgress();
	if (!isChargedBefore && ability->IsCharged() && GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 1.5f, FColor::Yellow, FString::Printf(TEXT("Ability %s is charged"), *GetName()));
	}
}

void ABattlemageTheEndlessCharacter::OnRemoveGameplayEffectCallback(const FActiveGameplayEffect& EffectRemoved)
{
}

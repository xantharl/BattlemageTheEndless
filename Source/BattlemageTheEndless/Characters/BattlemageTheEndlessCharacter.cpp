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
#include <BattlemageTheEndless/Abilities/AttackBaseGameplayAbility.h>
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

	// init gas
	AbilitySystemComponent = CreateDefaultSubobject<UBMageAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);

	// Minimal Mode means that no GameplayEffects will replicate. They will only live on the Server. Attributes, GameplayTags, and GameplayCues will still replicate to us.
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);

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
}

void ABattlemageTheEndlessCharacter::GiveStartingEquipment()
{
	// init equipment map
	if (Equipment.Num() == 0)
	{
		Equipment.Add(EquipSlot::Primary, FPickups());
		Equipment.Add(EquipSlot::Secondary, FPickups());
	}

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

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(Controller->InputComponent))
	{
		// update the last control input vector
		UE_LOG(LogTemplateCharacter, Log, TEXT("'%s' Updating Last Control Input Vector"), *GetNameSafe(this));
	}

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

	if (Controller != nullptr)
	{
		TObjectPtr<UBMageCharacterMovementComponent> movement = Cast<UBMageCharacterMovementComponent>(GetCharacterMovement());
		// add movement, only apply lateral if not wall running
		if (!movement->IsAbilityActive(MovementAbilityType::WallRun))
		{
			AddMovementInput(GetActorRightVector(), MovementVector.X);
		}

		AddMovementInput(GetActorForwardVector(), MovementVector.Y);

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

	// Set the active spell to the first granted ability if it is not already set and the item is a spell focus
	if (pickup->Weapon->SlotType == EquipSlot::Secondary && pickup->Weapon->GrantedAbilities.Num() > 0 && !pickup->Weapon->ActiveAbility)
	{
		pickup->Weapon->ActiveAbility = pickup->Weapon->GrantedAbilities[0];
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
		EquipmentBindingHandles.Add(pickup, FBindingHandles());
		auto handle = EnhancedInputComponent->BindAction(pickup->Weapon->FireAction, ETriggerEvent::Triggered,
			this, &ABattlemageTheEndlessCharacter::ProcessInputAndBindAbilityCancelled, pickup, EAttackType::Light).GetHandle();
		EquipmentBindingHandles[pickup].Handles.Add(handle);
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

// TODO: Clean up this excessive amount of nesting
void ABattlemageTheEndlessCharacter::ProcessInputAndBindAbilityCancelled(APickupActor* PickupActor, EAttackType AttackType)
{
	auto abilityHandle = ComboManager->ProcessInput(PickupActor, EAttackType::Light);

	// if the ability activated, check if we need to bind the cancel event
	if (abilityHandle.IsValid())
	{
		auto spec = AbilitySystemComponent->FindAbilitySpecFromHandle(abilityHandle);
		auto ability = Cast<UAttackBaseGameplayAbility>(AbilitySystemComponent->FindAbilitySpecFromHandle(abilityHandle)->Ability);

		// If the ability has projectiles to spawn, spawn them and exit
		//	Currently an ability can apply effects either on hit or to self, but not both
		if (ability->ProjectileConfiguration.ProjectileClass)
		{
			HandleProjectileSpawn(ability);
			return;
		}

		// If there are no effects, we have no need for a cancel callback
		if (ability->EffectsToApply.Num() == 0)
			return;

		// Otherwise bind the cancel event to remove the active effects immediately
		AbilitySystemComponent->OnAbilityEnded.AddUObject(this, &ABattlemageTheEndlessCharacter::OnAbilityCancelled);
	}
}

void ABattlemageTheEndlessCharacter::HandleProjectileSpawn(UAttackBaseGameplayAbility* ability)
{
	// TODO: Remove the return value if we end up not needing it
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
		GetCapsuleComponent()->IgnoreActorWhenMoving(projectile, true);
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

	// Not used currently
	bool durationEffectsApplied = false;

	// apply effects to the hit actor (this character)
	// TODO: maybe package instigator with the ability so we can trace back to the source of the ability
	ability->ApplyEffects(this, durationEffectsApplied, AbilitySystemComponent);
}
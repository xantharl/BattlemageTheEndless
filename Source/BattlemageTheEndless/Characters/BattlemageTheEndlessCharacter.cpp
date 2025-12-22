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
	if (auto AscComp = GetComponentByClass<UBMageAbilitySystemComponent>(); !AscComp)
	{
		AbilitySystemComponent = CreateDefaultSubobject<UBMageAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
		AbilitySystemComponent->SetIsReplicated(true);

		// See details of replication modes https://github.com/tranek/GASDocumentation?tab=readme-ov-file#concepts-asc-rm
		// Defaulting to minimal, updates to Mixed on possession (which indicates this is a player character)
		AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);

		// Create the attribute set, this replicates by default
		// Adding it as a subobject of the owning actor of an AbilitySystemComponent
		// automatically registers the AttributeSet with the AbilitySystemComponent
		AttributeSet = CreateDefaultSubobject<UBaseAttributeSet>(TEXT("AttributeSet"));
	}
	else if (!AbilitySystemComponent)
		AbilitySystemComponent = AscComp;
	
	CharacterCreationData = CreateDefaultSubobject<UCharacterCreationData>(TEXT("CharacterCreationData"));
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

		AbilitySystemComponent->IsLeftHanded = LeftHanded;
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
			GameMode->OnPlayerDied.Broadcast(this);
		}
	}
}

void ABattlemageTheEndlessCharacter::CallRestartPlayer()
{
	//Get the Controller of the Character.
	AController* controllerRef = GetController();
	if (LastCheckPoint)
	{
		auto Transform = LastCheckPoint->GetActorTransform();
		Transform.SetScale3D(SpawnTransform.GetScale3D());
		SetActorTransform(Transform);
	}
	else
	{		
		SetActorTransform(SpawnTransform);
	}

	//// Unpossess the controller before destroying the pawn, required to prevent nullptr Controller references.
	//if (controllerRef && controllerRef->GetPawn() == this)
	//{
	//	controllerRef->UnPossess();
	//}

	////Destroy the Player.   
	//Destroy();

	////Destroy the Player's equipment
	//for (auto& slot : Equipment)
	//{
	//	for (auto& pickup : slot.Value.Pickups)
	//	{
	//		pickup->Destroy();
	//	}
	//}

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

void ABattlemageTheEndlessCharacter::ProcessCharacterCreationData(TArray<FString> ArchetypeNames,TArray<FString> SpellNames)
{
	if (CharacterCreationData->SelectedPickupActors.Num() > 0)
	{
		// already processed
		return;
	}
	
	CharacterCreationData->InitSelections(ArchetypeNames, SpellNames);
}

void ABattlemageTheEndlessCharacter::BeginPlay()
{
	CheckRequiredObjects();

	TObjectPtr<UBMageCharacterMovementComponent> movement = Cast<UBMageCharacterMovementComponent>(GetCharacterMovement());
	if (movement)
	{
		movement->InitAbilities(this, GetMesh());
		AttributeSet->InitMovementSpeed(movement->MaxWalkSpeed);
		AttributeSet->InitCrouchedSpeed(movement->MaxWalkSpeedCrouched);
	}
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}
	
	GiveDefaultAbilities();
	GiveStartingEquipment();

	AttributeSet->OnAttributeChanged.AddDynamic(this, &ABattlemageTheEndlessCharacter::OnAttributeChanged);

	InitHealthbar();
	SpawnTransform = GetActorTransform();
	Super::BeginPlay();
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
		if (CharacterCreationData && CharacterCreationData->SelectedPickupActors.Num() > 0)
		{
			if (!CharacterCreationData->SelectedPickupActors.Contains(PickupType))
				continue;
		}
		
		// create the actor
		APickupActor* pickup = GetWorld()->SpawnActor<APickupActor>(PickupType, GetActorLocation(), GetActorRotation());
		pickup->SetOwner(this);

		if (pickup->Weapon)
			AbilitySystemComponent->OnAbilityEnded.AddUObject(pickup->Weapon, &UTP_WeaponComponent::OnAbilityCancelled);

		if (!pickup || !pickup->Weapon)
			continue;

		// disable collision so we won't block the player
		pickup->BaseCapsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		pickup->BaseCapsule->OnComponentBeginOverlap.RemoveAll(this);

		// Attach the weapon to the appropriate socket
		bool isRightHand = pickup->Weapon->SlotType == EquipSlot::Primary != LeftHanded;
		FName socketName = isRightHand ? FName("GripRight") : FName("GripLeft");
		pickup->Weapon->AttachToComponent(GetMesh(), FAttachmentTransformRules(EAttachmentRule::SnapToTarget, true), socketName);

		// TODO: deprecate this -- offset the weapon from the socket if needed
		if (pickup->Weapon->AttachmentOffset != FVector::ZeroVector && pickup->Weapon->GetRelativeLocation() == FVector::ZeroVector)
			pickup->Weapon->AddLocalOffset(pickup->Weapon->AttachmentOffset);

		if (pickup->Weapon->AttachmentTransform.IsValid())
			pickup->Weapon->AddLocalTransform(pickup->Weapon->AttachmentTransform);

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
	if (GetLocalRole() != ROLE_Authority || !IsValid(AbilitySystemComponent))
	{
		return;
	}
	
	if (!AbilitySystemComponent)
	{
		UE_LOG(LogTemplateCharacter, Error, TEXT("'%s' Failed to find an Ability System Component!"), *GetNameSafe(this));
		return;
	}

	// add default abilities
	for (auto Ability : DefaultAbilities)
	{
		AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(Ability.Key, 1, static_cast<int32>(EGASAbilityInputId::Confirm), this));

		// Abilities with input actions will not be activated immediately
		if (Ability.Value)
			continue;

		// Try to activate abilities that don't require input
		AbilitySystemComponent->TryActivateAbilityByClass(Ability.Key);
	}
}

void ABattlemageTheEndlessCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// Set up action bindings
	UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!EnhancedInputComponent)
	{
		UE_LOG(LogTemplateCharacter, Error, TEXT("'%s' Failed to find an Enhanced Input Component!"), *GetNameSafe(this));
		return;
	}

	//EnhancedInputComponent->BindAction(MenuAction, ETriggerEvent::Triggered, this, &ABattlemageTheEndlessCharacter::ToggleMenu);

	// Moving
	EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ABattlemageTheEndlessCharacter::Move);

	// Looking
	EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ABattlemageTheEndlessCharacter::Look);

	// Switch Camera
	EnhancedInputComponent->BindAction(SwitchCameraAction, ETriggerEvent::Triggered, this, &ABattlemageTheEndlessCharacter::SwitchCamera);

	// Dodge Actions
	//EnhancedInputComponent->BindAction(DodgeAction, ETriggerEvent::Triggered, this, &ABattlemageTheEndlessCharacter::DodgeInput);

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

	// Handle Default Ability Bindings
	for (auto entry : DefaultAbilities)
	{
		if (!entry.Value)
			continue;
		
		EnhancedInputComponent->BindAction(entry.Value, ETriggerEvent::Triggered, this, &ABattlemageTheEndlessCharacter::AbilityInputPressed, entry.Key);
		EnhancedInputComponent->BindAction(entry.Value, ETriggerEvent::Completed, this, &ABattlemageTheEndlessCharacter::AbilityInputReleased, entry.Key);
	}

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

void ABattlemageTheEndlessCharacter::Crouch(bool bClientSimulation)
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

	ACharacter::Crouch(bClientSimulation);
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

void ABattlemageTheEndlessCharacter::AbilityInputPressed(TSubclassOf<class UGameplayAbility> ability)
{
	auto attributeSet = Cast<UBaseAttributeSet>(AbilitySystemComponent->GetAttributeSet(UBaseAttributeSet::StaticClass()));
	auto speedBefore = attributeSet->GetMovementSpeed();
	if (AbilitySystemComponent->FindAbilitySpecFromClass(ability)->IsActive())
		return;

	auto success = AbilitySystemComponent->TryActivateAbilityByClass(ability);
	auto speedAfter = attributeSet->GetMovementSpeed();
	/*if (success && GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, FString::Printf(TEXT("Activated ability %s"), *ability->GetName()));
	}*/
}

void ABattlemageTheEndlessCharacter::AbilityInputReleased(TSubclassOf<class UGameplayAbility> ability)
{
	// Nothing to do if it's not active
	if (!AbilitySystemComponent->FindAbilitySpecFromClass(ability)->IsActive())
		return;
	// Do not end the ability if it has duration effects, that's up to the ability to end itself
	if (auto abilityWithEffects = Cast<UGA_WithEffectsBase>(ability->GetDefaultObject()))
	{
		auto hasDurationEffects = abilityWithEffects->EffectsToApply.FilterByPredicate([](TSubclassOf<UGameplayEffect> effect) {
			return effect.GetDefaultObject()->DurationPolicy == EGameplayEffectDurationType::HasDuration;
		}).Num() > 0;
		if (hasDurationEffects)
			return;
	}

	AbilitySystemComponent->CancelAbility(ability->GetDefaultObject<UGameplayAbility>());
	// if (GEngine)
	// {
	// 	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, FString::Printf(TEXT("Canceled ability %s"), *ability->GetName()));
	// }
}

void ABattlemageTheEndlessCharacter::EndSprint()
{
	if (TObjectPtr<UBMageCharacterMovementComponent> movement = Cast<UBMageCharacterMovementComponent>(GetCharacterMovement()))
	{
		movement->TryEndAbility(MovementAbilityType::Sprint);
	}
}

void ABattlemageTheEndlessCharacter::RequestUnCrouch()
{
	if (UBMageCharacterMovementComponent* mageMovement = Cast<UBMageCharacterMovementComponent>(GetCharacterMovement()))
		mageMovement->RequestUnCrouch();
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

void ABattlemageTheEndlessCharacter::CheckRequiredObjects()
{
	// This is kind of a hack, for AI the attributes are getting set in the CTOR but somehow becoming null by BeginPlay
	if (!AttributeSet)
		AttributeSet = NewObject<UBaseAttributeSet>(this, TEXT("AttributeSet"));
	if (!CharacterCreationData)
		CharacterCreationData = NewObject<UCharacterCreationData>(this, TEXT("CharacterCreationData"));
	
	AttributeSet->InitHealth(MaxHealth);
	AttributeSet->InitMaxHealth(MaxHealth);
	AttributeSet->InitHealthRegenRate(HealthRegenRate);
	TArray<FGameplayAttribute> attrSets;

	// The attributes are failing to init for AI, so we'll do it manually if needed
	AbilitySystemComponent->GetAllAttributes(attrSets);
	if (attrSets.Num() == 0)
		AbilitySystemComponent->AddAttributeSetSubobject(AttributeSet);
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

void ABattlemageTheEndlessCharacter::Jump()
{
	// get movement component as BMageCharacterMovementComponent
	UBMageCharacterMovementComponent* mageMovement = Cast<UBMageCharacterMovementComponent>(GetCharacterMovement());
	mageMovement->HandleJump(GetActiveCamera());
	Super::Jump();
}

void ABattlemageTheEndlessCharacter::SetActivePickup(APickupActor* pickup)
{
	if (!pickup || !pickup->Weapon)
		return;

	bool isRightHand = pickup->Weapon->SlotType == EquipSlot::Primary != LeftHanded;

	APickupActor* currentItem = isRightHand ? RightHandWeapon : LeftHandWeapon;
	UEnhancedInputComponent* EnhancedInputComponent = Controller != nullptr ? Cast<UEnhancedInputComponent>(Controller->InputComponent) : nullptr;

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
		if (EnhancedInputComponent)
		{
			for (auto handle : EquipmentBindingHandles[currentItem].Handles)
			{
				EnhancedInputComponent->RemoveBindingByHandle(handle);
			}
		}

		EquipmentBindingHandles.Remove(currentItem);
		EquipmentAbilityHandles[currentItem->Weapon->SlotType].Handles.Empty();

		if (currentItem->GrantedTags.Num() > 0)
		{
			AbilitySystemComponent->RemoveLooseGameplayTags(currentItem->GrantedTags);
		}

		AbilitySystemComponent->DeactivatePickup(currentItem);
	}

	// unhide newly activated weapon
	pickup->SetHidden(false);
	pickup->Weapon->SetHiddenInGame(false);

	// set it to the appropriate hand
	isRightHand ? RightHandWeapon = pickup : LeftHandWeapon = pickup;

	if (CharacterCreationData)
		AbilitySystemComponent->ActivatePickup(pickup, CharacterCreationData->SelectedSpells);
	else
		AbilitySystemComponent->ActivatePickup(pickup);

	// to avoid nullptr
	if (!pickup->Weapon->SelectedAbility && pickup->Weapon->GrantedAbilities.Num() > 0)
	{
		pickup->Weapon->SelectedAbility = pickup->Weapon->GrantedAbilities[0];
	}

	if (pickup->GrantedTags.Num() > 0)
	{
		AbilitySystemComponent->AddLooseGameplayTags(pickup->GrantedTags);
	}

	// track key bindings for the new weapon
	if (EnhancedInputComponent)
	{
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

	// If we are on the last spell for this class, activate the next spell class instead
	if (nextOrPrevious && ActiveSpellClass->Weapon->GrantedAbilities.Last() == ActiveSpellClass->Weapon->SelectedAbility)
	{
		const int CurrentIndex = Equipment[EquipSlot::Secondary].Pickups.IndexOfByKey(ActiveSpellClass);
		const int NextIndex = (CurrentIndex + 1) % Equipment[EquipSlot::Secondary].Pickups.Num();
		ActiveSpellClass = Equipment[EquipSlot::Secondary].Pickups[NextIndex];
		SetActivePickup(ActiveSpellClass);
		ActiveSpellClass->Weapon->ChangeAbilityToIndex(0, true);
		return;
	}
		
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

bool ABattlemageTheEndlessCharacter::HasWeapon(EquipSlot SlotType)
{
	return GetWeapon(SlotType) != nullptr;
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
		if (!movement->IsAbilityActive(_sprintTag))
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

void ABattlemageTheEndlessCharacter::OnAttributeChanged(const FGameplayAttribute& Attribute, float OldValue, float NewValue)
{
	FOnAttributeChangeData Data;
	Data.Attribute = Attribute;
	Data.OldValue = OldValue;
	Data.NewValue = NewValue;

	if (Attribute == AttributeSet->GetHealthAttribute())
	{
		OnHealthChanged(Data);
	}
	if (Attribute == AttributeSet->GetHealthRegenRateAttribute())
	{
		OnHealthRegenRateChanged(Data);
	}
	else if (Attribute == AttributeSet->GetMovementSpeedAttribute())
	{
		OnMovementSpeedChanged(Data);
	}
	else if (Attribute == AttributeSet->GetCrouchedSpeedAttribute())
	{
		OnCrouchedSpeedChanged(Data);
	}
}

void ABattlemageTheEndlessCharacter::DestroyDeadCharacter()
{
	Destroy();
	// Destroy the Enemy's equipment
	for (auto& slot : Equipment)
	{
		for (auto& pickup : slot.Value.Pickups)
		{
			pickup->Destroy();
		}
	}
}

void ABattlemageTheEndlessCharacter::OnHealthChanged(const FOnAttributeChangeData& Data)
{
	// If already dead and health is still zero or below, do nothing
	if (IsDead && Data.NewValue <= 0.f)
		return;
	
	// If health isn't set up, exit
	if (AttributeSet->GetMaxHealth() == 0)
		return;

	// this will eventually be more nuanced, but for now just suspend regen on any damage taken
	if (Data.NewValue < Data.OldValue)
		AbilitySystemComponent->SuspendHealthRegen();

	AbilitySystemComponent->UpdateShouldTick();

	// If health is zero or below, die
	if (Data.NewValue <= 0.f)
	{
		IsDead = true;
		OnIsDeadChanged.Broadcast(true);
		
		auto controller = GetController();
		if (controller && controller->IsPlayerController())
		{
			CallRestartPlayer();
		}
		else
		{
			if (TimeToPersistAfterDeath > 0.f)
			{
				FTimerDelegate TimerDelegate = FTimerDelegate::CreateUObject(this, &ABattlemageTheEndlessCharacter::DestroyDeadCharacter);
				GetWorld()->GetTimerManager().SetTimer(PersistAfterDeathTimerHandle, TimerDelegate, TimeToPersistAfterDeath, false);
				return;
			}
			
			DestroyDeadCharacter();
		}
	}
}

void ABattlemageTheEndlessCharacter::OnHealthRegenRateChanged(const FOnAttributeChangeData& Data)
{
	// When we change health regen rate, we may need to start or stop ticking
	AbilitySystemComponent->UpdateShouldTick();
}

void ABattlemageTheEndlessCharacter::OnMovementSpeedChanged(const FOnAttributeChangeData& Data)
{
	if (auto movement = GetCharacterMovement())
	{
		movement->MaxWalkSpeed = Data.NewValue;
	}
}

void ABattlemageTheEndlessCharacter::OnCrouchedSpeedChanged(const FOnAttributeChangeData& Data)
{
	if (auto movement = GetCharacterMovement())
	{
		movement->MaxWalkSpeedCrouched = Data.NewValue;
	}
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
				AbilitySystemComponent->ProcessInputAndBindAbilityCancelled(PickupActor, AttackType);
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
	auto isComboActive = AbilitySystemComponent->ComboManager->Combos.Contains(PickupActor) && AbilitySystemComponent->ComboManager->Combos[PickupActor].ActiveCombo;

	if (ability && (ability->ChargeDuration > 0.001f || (ability->HitType == HitType::Placed && !isComboActive)))
	{
		// if the ability is charged or placed and we aren't in a combo, this is the wrong handler
		return;
	}

	/*GEngine->AddOnScreenDebugMessage(-1, 1.50f, FColor::Blue, FString::Printf(TEXT("ProcessSpellInput: Ability %s"),
		*(ability->GetAbilityName().ToString())));*/

	AbilitySystemComponent->ProcessInputAndBindAbilityCancelled(PickupActor, AttackType);
}

void ABattlemageTheEndlessCharacter::ProcessSpellInput_Charged(APickupActor* PickupActor, EAttackType AttackType, ETriggerEvent triggerEvent)
{
	if (triggerEvent != ETriggerEvent::Started && triggerEvent != ETriggerEvent::Completed)
	{
		// we only handle started and completed events for charged abilities
		return;
	}

	auto SelectedAbilitySpec = AbilitySystemComponent->FindAbilitySpecFromClass(ActiveSpellClass->Weapon->SelectedAbility);
	if (!SelectedAbilitySpec)
	{
		UE_LOG(LogTemplateCharacter, Warning, TEXT("'%s' ProcessSpellInput_Charged called without a valid Ability Spec!"), *GetNameSafe(this));
		return;
	}
	
	auto Ability = Cast<UAttackBaseGameplayAbility>(SelectedAbilitySpec->Ability);

	if (!Ability || Ability->ChargeDuration <= 0.0001f)
	{
		// if the ability is not charged, this is the wrong handler
		return;
	}

	TObjectPtr<UGameplayAbility> activeInstance = SelectedAbilitySpec->GetPrimaryInstance();
	auto attackAbility = Cast<UAttackBaseGameplayAbility>(activeInstance);
	if (!attackAbility)
		return;

	switch (triggerEvent)
	{
	case ETriggerEvent::Started:
	{
		AbilitySystemComponent->BeginChargeAbility(ActiveSpellClass->Weapon->SelectedAbility);
		break;
	}
	case ETriggerEvent::Completed:
	{
		AbilitySystemComponent->CompleteChargeAbility();
		break;
	}
	// We don't need to do anything for ongoing since we've got the repeating timer handling it on a set interval
	default:
		break;
	}
}

void ABattlemageTheEndlessCharacter::ProcessSpellInput_Placed(APickupActor* PickupActor, EAttackType AttackType, ETriggerEvent triggerEvent)
{
	const auto SelectedAbilitySpec = AbilitySystemComponent->FindAbilitySpecFromClass(ActiveSpellClass->Weapon->SelectedAbility);
	if (!SelectedAbilitySpec)
	{
		UE_LOG(LogTemplateCharacter, Warning, TEXT("'%s' ProcessSpellInput_Placed called without a valid Ability Spec!"), *GetNameSafe(this));
		return;
	}
	
	const auto Ability = Cast<UAttackBaseGameplayAbility>(SelectedAbilitySpec->Ability);

	if (!Ability || Ability->HitType != HitType::Placed)
	{
		// if the ability is not placed, this is the wrong handler
		return;
	}

	_secondsSinceLastPrint += GetWorld()->GetTime().GetDeltaWorldTimeSeconds();
	if (_secondsSinceLastPrint > 0.33f)
	{
	/*	GEngine->AddOnScreenDebugMessage(-1, 1.50f, FColor::Blue, FString::Printf(TEXT("ProcessSpellInput_Placed: Ability %s"),
			*(ability->GetAbilityName().ToString())));*/

		_secondsSinceLastPrint = 0;
	}

	// check if this ability has a combo and we are past the first part
	if (AbilitySystemComponent->ComboManager->Combos.Contains(PickupActor) && AbilitySystemComponent->ComboManager->Combos[PickupActor].ActiveCombo)
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
			Ability->SpawnSpellActors(true, this);
			break;
		}
		// The button is being held
		case ETriggerEvent::Ongoing:
		{
			for (auto ghostActor : Ability->GetPlacementGhosts())
			{
				if (ghostActor && IsValid(ghostActor))
				{
					Ability->PositionSpellActor(ghostActor, this);
				}
			}
			break;
		}
		// Casting was cancelled
		case ETriggerEvent::Canceled:
		{
			// destroy all placement ghosts
			for (auto ghostActor : Ability->GetPlacementGhosts())
				ghostActor->Destroy();
			break;
		}
		// Casting was confirmed by releasing button
		case ETriggerEvent::Triggered:
		{			
			AbilitySystemComponent->ComboManager->ProcessInput(PickupActor, AttackType);
			// reset materials on all placement ghosts
			for (auto ghostActor : Ability->GetPlacementGhosts())
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
			Ability->ClearPlacementGhosts();

			break;
		}
		default:
			break;
	}
}

void ABattlemageTheEndlessCharacter::OnProjectileHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	ABattlemageTheEndlessProjectile* projectile = Cast<ABattlemageTheEndlessProjectile>(OtherActor);
	if (!projectile)
		return;
	
	const UAttackBaseGameplayAbility* ability = Cast<UAttackBaseGameplayAbility>(projectile->SpawningAbility);
	if (!ability)
		return;

	// apply effects to the hit actor (this character)
	ability->ApplyEffects(this, AbilitySystemComponent, nullptr, projectile);
	if (projectile->ShouldDestroyOnHit())
		projectile->Destroy();
}
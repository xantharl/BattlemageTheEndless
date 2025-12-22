// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "GameFramework/CharacterMovementComponent.h" 
#include "BMageCharacterMovementComponent.h"
#include "GameplayCueNotify_Actor.h"
#include <GameFramework/SpringArmComponent.h>
#include "../Pickups/TP_WeaponComponent.h"
#include "Camera/CameraComponent.h"
#include "../Helpers/Traces.h"
#include "cmath"
#include <chrono>
#include <map>
#include "../Helpers/VectorMath.h"
#include "../GameMode/BattlemageTheEndlessGameMode.h"
#include "../GameMode/CheckPoint.h"
#include "Widgets/SViewport.h"
#include "CommonActivatableWidget.h"
#include "Widgets/CommonActivatableWidgetContainer.h"
#include "Widgets/SWidget.h"
#include "BattlemageTheEndless/Widgets/MenuContainerActivatableWidget.h"
#include "BattlemageTheEndless/Characters/BattlemageTheEndlessPlayerController.h"
#include "../BMageAbilitySystemComponent.h"
#include "Blueprint/WidgetTree.h"
#include "../Pickups/PickupActor.h"
#include "../BMageJumpAbility.h"
#include <BattlemageTheEndless/Characters/ComboManagerComponent.h>
#include <BattlemageTheEndless/Abilities/AttackBaseGameplayAbility.h>
#include "../Pickups/BattlemageTheEndlessProjectile.h"
#include <BattlemageTheEndless/Abilities/BaseAttributeSet.h>
#include "../Widgets/HealthBarWidget.h"
#include "Components/WidgetComponent.h"
#include "../Characters/ProjectileManagerComponent.h"
#include "BattlemageTheEndless/Character/CharacterCreationData.h"

#include "BattlemageTheEndlessCharacter.generated.h"

class UInputComponent;
class USkeletalMeshComponent;
class UInputAction;
class UInputMappingContext;
struct FInputActionValue;
using namespace std::chrono;

DECLARE_DELEGATE_OneParam(FSpellClassSelectDelegate, const int);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnIsDeadChanged, bool, IsDead);

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

// Needed for iteration in input mapping, this intentionally skips None
ENUM_RANGE_BY_FIRST_AND_LAST(ETriggerEvent, ETriggerEvent::Triggered, ETriggerEvent::Completed);

USTRUCT(BlueprintType)
struct FPickups
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Equipment, meta = (AllowPrivateAccess = "true"))
	TArray<TObjectPtr<APickupActor>> Pickups;
};

USTRUCT(BlueprintType)
struct FAbilityHandles
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Equipment, meta = (AllowPrivateAccess = "true"))
	TArray<FGameplayAbilitySpecHandle> Handles;
};

USTRUCT(BlueprintType)
struct FBindingHandles
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Equipment, meta = (AllowPrivateAccess = "true"))
	TArray<int32> Handles;
};

UCLASS(config=Game)
class ABattlemageTheEndlessCharacter : public ACharacter
{
	GENERATED_BODY()

private:
	FGameplayTag _sprintTag = FGameplayTag::RequestGameplayTag(FName("Movement.Sprint"));
	FGameplayTag _crouchTag = FGameplayTag::RequestGameplayTag(FName("Movement.Crouch"));
	FGameplayTag _slideTag = FGameplayTag::RequestGameplayTag(FName("Movement.Slide"));
	FGameplayTag _dodgeTag = FGameplayTag::RequestGameplayTag(FName("Movement.Dodge"));

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Abilities, meta = (AllowPrivateAccess = "true"))
	TMap<TSubclassOf<class UGameplayAbility>, UInputAction*> DefaultAbilities;

	/** Third person camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	UCameraComponent* ThirdPersonCamera;

	/** Third person camera boom */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* ThirdPersonCameraBoom;

	/** First person camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FirstPersonCamera;

	/** MappingContext */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess = "true"))
	UInputMappingContext* DefaultMappingContext;

	/** Launch Jump Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* DodgeAction;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess = "true"))
	UInputAction* MoveAction;

	/** Sprint Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* SwitchCameraAction;

	/** Sprint Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* DropItemAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* RespawnAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* MenuAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Input, meta = (AllowPrivateAccess = "true"))
	bool IsInCastingMode = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* CastingModeAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* SpellClassOneAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* SpellClassTwoAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* SpellClassThreeAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* SpellClassFourAction;

	// These next two actions are placeholders until I get a final spell selection system in place

	// Select the next spell in the equipped class
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* NextSpellAction;

	// Select the previous spell in the equipped class
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* PreviousSpellAction;

	// Aim mode action
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* AimAction;

	// Time to enter or exit aim mode
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = AimMode, meta = (AllowPrivateAccess = "true"))
	float TimeToAim = 0.2f;

	// Time to enter or exit aim mode
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = AimMode, meta = (AllowPrivateAccess = "true"))
	float FovUpdateInterval = 0.01f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AimMode, meta = (AllowPrivateAccess = "true"))
	float ZoomedFOV = 40.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AimMode, meta = (AllowPrivateAccess = "true"))
	float DefaultFOV = 40.0f;

	UPROPERTY(EditAnywhere, Category = Sound)
	class USoundWave* JumpLandingSound;

	UPROPERTY(EditAnywhere, Category = Sound)
	class USoundWave* RunningSound;

	UPROPERTY(EditAnywhere, Category = Sound)
	class USoundWave* SprintingSound;

	UPROPERTY(EditAnywhere, Category = Inventory)
	APickupActor* LeftHandWeapon;

	UPROPERTY(EditAnywhere, Category = Inventory)
	APickupActor* RightHandWeapon;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = UserInterface, meta = (AllowPrivateAccess = "true"))
	UMenuContainerActivatableWidget* ContainerWidget;

	/** Map of last activated abilities. This is NOT automatically cleared out on ability end, and will be validated on next attempt to process input **/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Abilities, meta = (AllowPrivateAccess = "true"))
	TMap<APickupActor*, FGameplayAbilitySpecHandle> LastActivatedAbilities;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CharacterState, meta = (AllowPrivateAccess = "true"))
	bool IsDead = false;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CharacterState, meta = (AllowPrivateAccess = "true"))
	float TimeToPersistAfterDeath = 10.f;
	
	UPROPERTY(BlueprintAssignable, Category = "Attributes")
	FOnIsDeadChanged OnIsDeadChanged;	
private:
	milliseconds _lastCameraSwap;

public:
	ABattlemageTheEndlessCharacter(const FObjectInitializer& ObjectInitializer);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Abilities, meta = (AllowPrivateAccess = "true"), Instanced)
	class UBMageAbilitySystemComponent* AbilitySystemComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Abilities, meta = (AllowPrivateAccess = "true"), Instanced)
	UBaseAttributeSet* AttributeSet;

	virtual void PossessedBy(AController* NewController) override;

	void SetupCapsule();

	//Called when our Actor is destroyed during Gameplay.
	virtual void Destroyed();

	//Call Gamemode class to Restart Player Character.
	void CallRestartPlayer();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Equipment, meta = (AllowPrivate))
	TMap<EquipSlot, FPickups> Equipment;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Equipment, meta = (AllowPrivate))
	TMap<EquipSlot, FAbilityHandles> EquipmentAbilityHandles;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Equipment, meta = (AllowPrivate))
	TMap<APickupActor*, FBindingHandles> EquipmentBindingHandles;

	APickupActor* ActiveSpellClass;

	UCameraComponent* GetActiveCamera() const {
		return FirstPersonCamera->IsActive() ? FirstPersonCamera : ThirdPersonCamera;
	}

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Equipment, meta = (AllowPrivate))
	TArray<TSubclassOf<class APickupActor>> DefaultEquipment;
	
	/** Character creation data set during character creation flow, use UFUNCTION ProcessCharacterCreationData to set it **/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CharacterCreation, meta = (AllowPrivate), Instanced)
	UCharacterCreationData* CharacterCreationData;
	
	UFUNCTION(BlueprintCallable, Category = CharacterCreation)
	void ProcessCharacterCreationData(TArray<FString> ArchetypeNames,TArray<FString> SpellNames);
	
protected:
	virtual void BeginPlay();
	void InitHealthbar();

	// currently only enemies inheriting from this class have a health bar, so this can return nullptr
	UHealthBarWidget* GetHealthBarWidget();

	void GiveStartingEquipment();
	void GiveDefaultAbilities();
	virtual void OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode = 0);

	void SetupCameras();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Attributes)
	float Health = 100;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Attributes)
	float MaxHealth = 100;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Attributes)
	float HealthRegenRate = 2.f;

	ACheckPoint* LastCheckPoint;

	FDelegateHandle HealthChangedDelegateHandle;

	void ToggleAimMode(ETriggerEvent triggerType);
	void AdjustAimModeFov(float DeltaTime);

	bool IsAiming = false;
	FTimerHandle AimModeTimerHandle;
	FTimerHandle PersistAfterDeathTimerHandle;

public:
	/** Look Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* LookAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	float SlideBrakingFactor = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CharacterMovement, meta = (AllowPrivateAccess = "true"))
	float BaseHalfHeight = 90.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CharacterMovement, meta = (AllowPrivateAccess = "true"))
	float DoubleJumpHorizontalWeight = 1.5f;

	UFUNCTION(BlueprintCallable, Category = Weapon)
	APickupActor* GetWeapon(EquipSlot SlotType);

	UFUNCTION(BlueprintCallable, Category = Weapon)
	bool HasWeapon(EquipSlot SlotType);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Inventory, meta = (AllowPrivateAccess = "true"))
	bool LeftHanded = false;

	FName GetTargetSocketName(EquipSlot SlotType);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Casting, meta = (AllowPrivateAccess = "true"))
	UAnimMontage* CastingModeMontage;

	UFUNCTION(BlueprintCallable, Category = Equipment)
	void EquipSpellClass(int slotNumber);

	// Method called by Niagara specific impl of actor gameplay cue
	bool TryRemovePreviousAbilityEffect(AGameplayCueNotify_Actor* Notify);

	virtual void PawnClientRestart();

	virtual void Crouch(bool bClientSimulation);

	UFUNCTION(BlueprintCallable, Category = "Input")
	void RequestUnCrouch();

	UFUNCTION(BlueprintCallable, Category = "Input")
	UBMageCharacterMovementComponent* GetBMageMovementComponent() { return Cast<UBMageCharacterMovementComponent>(GetCharacterMovement()); }

	// This method checks that all required objects are set and valid, and creates them if not
	// NOTE: This isn't needed for the player character, but objects set in the CTOR are becoming null by BeginPlay for AI characters
	void CheckRequiredObjects();

protected:
	/** Called for movement input */
	void Move(const FInputActionValue& Value);

	/** Called for looking input */
	void Look(const FInputActionValue& Value);

	// this is an override but specifying it as such breaks compile for some reason
	void Jump();

	UFUNCTION(BlueprintCallable, Category = Weapon)
	void SetActivePickup(APickupActor* pickup);
	// True = next, False = previous
	UFUNCTION(BlueprintCallable, Category = Weapon)
	void SelectActiveSpell(bool nextOrPrevious);

	// APawn interface
	virtual void SetupPlayerInputComponent(UInputComponent* InputComponent) override;
	void EndSprint();
	void TickActor(float DeltaTime, ELevelTick TickType, FActorTickFunction& ThisTickFunction);
	void DoUnCrouch(UBMageCharacterMovementComponent* movement);
	void EndSlide(UCharacterMovementComponent* movement);
	void AbilityInputPressed(TSubclassOf<class UGameplayAbility> ability);
	void AbilityInputReleased(TSubclassOf<class UGameplayAbility> ability);
	void SwitchCamera();
	void DodgeInput();
	void ToggleCastingMode();

	bool IsDodging();

	UFUNCTION(BlueprintCallable, Category = "CheckPoint")
	void OnBaseCapsuleBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	virtual void OnAttributeChanged(const FGameplayAttribute& Attribute, float OldValue, float NewValue);
	void DestroyDeadCharacter();
	virtual void OnHealthChanged(const FOnAttributeChangeData& Data);
	virtual void OnHealthRegenRateChanged(const FOnAttributeChangeData& Data);
	virtual void OnMovementSpeedChanged(const FOnAttributeChangeData& Data);
	virtual void OnCrouchedSpeedChanged(const FOnAttributeChangeData& Data);

	void ProcessMeleeInput(APickupActor* PickupActor, EAttackType AttackType, ETriggerEvent triggerEvent);

	/** Spell specific handler which decides whether to call ProcessInputAndBindAbilityCancelled or do nothing **/
	void ProcessSpellInput(APickupActor* PickupActor, EAttackType AttackType, ETriggerEvent triggerEvent);
	void ProcessSpellInput_Charged(APickupActor* PickupActor, EAttackType AttackType, ETriggerEvent triggerEvent);
	void ProcessSpellInput_Placed(APickupActor* PickupActor, EAttackType AttackType, ETriggerEvent triggerEvent);

	UFUNCTION(BlueprintCallable, Category = "Projectile")
	void OnProjectileHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	const int TickRate = 60;

	// Debug time to avoid spamming print messages
	float _secondsSinceLastPrint = 0;
	
	FTransform SpawnTransform;
};


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
#include <BattlemageTheEndless/Abilities/Combos/AbilityComboManager.h>
#include <BattlemageTheEndless/Abilities/BaseAttributeSet.h>
#include "../Abilities/ProjectileManager.h"

#include "BattlemageTheEndlessCharacter.generated.h"

class UInputComponent;
class USkeletalMeshComponent;
class UCameraComponent;
class UInputAction;
class UInputMappingContext;
struct FInputActionValue;
using namespace std::chrono;

DECLARE_DELEGATE_OneParam(FSpellClassSelectDelegate, const int);

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

UENUM(BlueprintType)
enum class EGASAbilityInputId : uint8
{
	None UMETA(DisplayName = "None"),
	Confirm UMETA(DisplayName = "Confirm"),
	Cancel UMETA(DisplayName = "Cancel")
};

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

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = Abilities, meta = (AllowPrivateAccess = "true"))
	TArray<TSubclassOf<class UGameplayAbility>> DefaultAbilities;

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

	/** Jump Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess = "true"))
	UInputAction* JumpAction;

	/** Launch Jump Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* LaunchJumpAction;

	/** Launch Jump Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* DodgeAction;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess = "true"))
	UInputAction* MoveAction;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* CrouchAction;

	/** Sprint Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* SprintAction;

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

private:
	milliseconds _lastCameraSwap;
	milliseconds _lastJumpTime;

public:
	ABattlemageTheEndlessCharacter(const FObjectInitializer& ObjectInitializer);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Abilities, meta = (AllowPrivateAccess = "true"))
	class UBMageAbilitySystemComponent* AbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Abilities, meta = (AllowPrivateAccess = "true"))
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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Equipment, meta = (AllowPrivate))
	TArray<TSubclassOf<class APickupActor>> DefaultEquipment;

protected:
	virtual void BeginPlay();
	void GiveStartingEquipment();
	void GiveDefaultAbilities();
	virtual void OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode = 0);

	void SetupCameras();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Character)
	float Health = 100;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Character)
	float MaxHealth = 100;

	ACheckPoint* LastCheckPoint;

	FDelegateHandle HealthChangedDelegateHandle;

	/// <summary>
	/// Manages projectile spawn and hit for abilities
	/// </summary>
	UProjectileManager* ProjectileManager;
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UAbilityComboManager* ComboManager;

	/** Look Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* LookAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CharacterMovement, meta = (AllowPrivateAccess = "true"))
	bool bShouldUnCrouch;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CharacterMovement, meta = (AllowPrivateAccess = "true"))
	float BaseHalfHeight = 90.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CharacterMovement, meta = (AllowPrivateAccess = "true"))
	float DoubleJumpHorizontalWeight = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CharacterMovement, meta = (AllowPrivateAccess = "true"))
	bool ApplyMovementInputToJump = true;

	/** Jump Cooldown expressed in seconds**/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CharacterMovement, meta = (AllowPrivateAccess = "true"))
	double JumpCooldown = 0.05;

	UFUNCTION(BlueprintCallable, Category = Weapon)
	APickupActor* GetWeapon(EquipSlot SlotType);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Inventory, meta = (AllowPrivateAccess = "true"))
	bool LeftHanded = false;

	FName GetTargetSocketName(EquipSlot SlotType);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Casting, meta = (AllowPrivateAccess = "true"))
	UAnimMontage* CastingModeMontage;

	FVector CurrentGripOffset(FName SocketName);

	UFUNCTION(BlueprintCallable, Category = Equipment)
	void EquipSpellClass(int slotNumber);

	// Method called by Niagara specific impl of actor gameplay cue
	bool TryRemovePreviousAbilityEffect(AGameplayCueNotify_Actor* Notify);

	virtual void PawnClientRestart();

protected:
	/** Called for movement input */
	void Move(const FInputActionValue& Value);

	/** Called for looking input */
	void Look(const FInputActionValue& Value);

	void Jump();

	void RedirectVelocityToLookDirection(bool wallrunEnded);

	void SetActivePickup(APickupActor* pickup);
	// True = next, False = previous
	void SelectActiveSpell(bool nextOrPrevious);

	// APawn interface
	virtual void SetupPlayerInputComponent(UInputComponent* InputComponent) override;
	void Crouch();
	void StartSprint();
	void EndSprint();
	void LaunchJump();
	void RequestUnCrouch();
	void TickActor(float DeltaTime, ELevelTick TickType, FActorTickFunction& ThisTickFunction);
	void DoUnCrouch(UCharacterMovementComponent* movement);
	void EndSlide(UCharacterMovementComponent* movement);
	void SwitchCamera();
	void DodgeInput();
	void ToggleCastingMode();

	bool IsDodging();

	UFUNCTION(BlueprintCallable, Category = "CheckPoint")
	void OnBaseCapsuleBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	virtual void HealthChanged(const FOnAttributeChangeData& Data);

	virtual void ProcessInputAndBindAbilityCancelled(APickupActor* PickupActor, EAttackType AttackType);

	void HandleProjectileSpawn(UAttackBaseGameplayAbility* ability);
	void HandleHitScan(UAttackBaseGameplayAbility* ability);
	TArray<ABattlemageTheEndlessCharacter*> GetChainTargets(int NumberOfChains, float ChainDistance, AActor* HitActor);
	ABattlemageTheEndlessCharacter* GetNextChainTarget(float ChainDistance, AActor* ChainActor, TArray<AActor*> Candidates);

	void OnAbilityCancelled(const FAbilityEndedData& endData);

	UFUNCTION(BlueprintCallable, Category = "Projectile")
	void OnProjectileHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);
};


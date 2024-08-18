// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "GameFramework/CharacterMovementComponent.h" 
#include "BMageCharacterMovementComponent.h"
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
#include "Blueprint/WidgetTree.h"

#include "BattlemageTheEndlessCharacter.generated.h"

class UInputComponent;
class USkeletalMeshComponent;
class UCameraComponent;
class UInputAction;
class UInputMappingContext;
struct FInputActionValue;
using namespace std::chrono;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

UCLASS(config=Game)
class ABattlemageTheEndlessCharacter : public ACharacter
{
	GENERATED_BODY()

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

	UPROPERTY(EditAnywhere, Category = Sound)
	class USoundWave* JumpLandingSound;

	UPROPERTY(EditAnywhere, Category = Sound)
	class USoundWave* RunningSound;

	UPROPERTY(EditAnywhere, Category = Sound)
	class USoundWave* SprintingSound;

	UPROPERTY(EditAnywhere, Category = Inventory)
	UTP_WeaponComponent* LeftHandWeapon;

	UPROPERTY(EditAnywhere, Category = Inventory)
	UTP_WeaponComponent* RightHandWeapon;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = UserInterface, meta = (AllowPrivateAccess = "true"))
	UMenuContainerActivatableWidget* ContainerWidget;

private:
	milliseconds _lastCameraSwap;
	milliseconds _lastJumpTime;

public:
	ABattlemageTheEndlessCharacter(const FObjectInitializer& ObjectInitializer);

	void SetupCapsule();

	//Called when our Actor is destroyed during Gameplay.
	virtual void Destroyed();

	//Call Gamemode class to Restart Player Character.
	void CallRestartPlayer();

	virtual void ApplyDamage(float damage);

protected:
	virtual void BeginPlay();
	virtual void OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode = 0);

	void OnJumpLanded();
	void SetupCameras();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Character)
	float Health = 100;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Character)
	float MaxHealth = 100;

	ACheckPoint* LastCheckPoint;

public:
		
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

	/** TODO: Handle this in c++ rather than BP **/
	UFUNCTION(BlueprintCallable, Category = Weapon)
	void SetLeftHandWeapon(UTP_WeaponComponent* weapon);

	/** TODO: Handle this in c++ rather than BP **/
	UFUNCTION(BlueprintCallable, Category = Weapon)
	void SetRightHandWeapon(UTP_WeaponComponent* weapon);

	UFUNCTION(BlueprintCallable, Category = Weapon)
	bool GetHasLeftHandWeapon();

	UFUNCTION(BlueprintCallable, Category = Weapon)
	bool GetHasRightHandWeapon();

	UFUNCTION(BlueprintCallable, Category = Weapon)
	UTP_WeaponComponent* GetWeapon(EquipSlot SlotType);

	/** TODO: Is this redundant since the property is public? **/
	UFUNCTION(BlueprintCallable, Category = Weapon)
	UTP_WeaponComponent* GetLeftHandWeapon();

	/** TODO: Is this redundant since the property is public? **/
	UFUNCTION(BlueprintCallable, Category = Weapon)
	UTP_WeaponComponent* GetRightHandWeapon();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Inventory, meta = (AllowPrivateAccess = "true"))
	bool LeftHanded = false;

	FName GetTargetSocketName(EquipSlot SlotType);

	bool TrySetWeapon(UTP_WeaponComponent* Weapon, FName SocketName);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Casting, meta = (AllowPrivateAccess = "true"))
	UAnimMontage* CastingModeMontage;

protected:
	/** Called for movement input */
	void Move(const FInputActionValue& Value);

	/** Called for looking input */
	void Look(const FInputActionValue& Value);

	void Jump();

	void RedirectVelocityToLookDirection(bool wallrunEnded);

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

	void ToggleMenu();
};


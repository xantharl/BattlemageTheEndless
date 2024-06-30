// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "GameFramework/CharacterMovementComponent.h" 
#include <GameFramework/SpringArmComponent.h>
#include "TP_WeaponComponent.h"
#include "cmath"
#include "BattlemageTheEndlessCharacter.generated.h"

class UInputComponent;
class USkeletalMeshComponent;
class UCameraComponent;
class UInputAction;
class UInputMappingContext;
struct FInputActionValue;

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

	/** The CapsuleComponent being used for movement collision (by CharacterMovement). Always treated as being vertically aligned in simple collision check functions. */
	UPROPERTY(Category = Character, VisibleAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCapsuleComponent> CrouchCapsuleComponent;

	/** The CapsuleComponent being used for movement collision (by CharacterMovement). Always treated as being vertically aligned in simple collision check functions. */
	UPROPERTY(Category = Character, VisibleAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCapsuleComponent> SlideCapsuleComponent;

	/** Jump Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess = "true"))
	UInputAction* JumpAction;

	/** Launch Jump Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* LaunchJumpAction;

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

private:
	time_t _lastCameraSwap;

public:
	ABattlemageTheEndlessCharacter();

	void SetupCameras();

	void SetupCollisionComponents();

	virtual void ApplyDamage(float damage);

protected:
	virtual void BeginPlay();
	virtual void OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode = 0);
	virtual void TickActor(float DeltaTime, enum ELevelTick TickType, FActorTickFunction& ThisTickFunction);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Character)
	float Health = 100;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Character)
	float MaxHealth = 100;

public:
		
	/** Look Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* LookAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CharacterMovement, meta = (AllowPrivateAccess = "true"))
	bool bIsSprinting;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CharacterMovement, meta = (AllowPrivateAccess = "true"))
	bool IsSliding;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CharacterMovement, meta = (AllowPrivateAccess = "true"))
	bool bShouldUncrouch;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CharacterMovement, meta = (AllowPrivateAccess = "true"))
	float SprintSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CharacterMovement, meta = (AllowPrivateAccess = "true"))
	float SlideSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CharacterMovement, meta = (AllowPrivateAccess = "true"))
	float WalkSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CharacterMovement, meta = (AllowPrivateAccess = "true"))
	float ReverseSpeed = 300.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CharacterMovement, meta = (AllowPrivateAccess = "true"))
	float BaseHalfHeight = 90.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CharacterMovement, meta = (AllowPrivateAccess = "true"))
	float CrouchedHalfHeight = 65.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CharacterMovement, meta = (AllowPrivateAccess = "true"))
	float SlideHalfHeight = 35.0f;

	// The default value is based on the slide animation length
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CharacterMovement, meta = (AllowPrivateAccess = "true"))
	float SlideDurationSeconds = 1.67f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CharacterMovement, meta = (AllowPrivateAccess = "true"))
	float CrouchSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CharacterMovement, meta = (AllowPrivateAccess = "true"))
	float LaunchSpeedHorizontal;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CharacterMovement, meta = (AllowPrivateAccess = "true"))
	float LaunchSpeedVertical;

	int MaxLaunches;

	UFUNCTION(BlueprintCallable, Category = Weapon)
	void SetLeftHandWeapon(UTP_WeaponComponent* weapon);

	UFUNCTION(BlueprintCallable, Category = Weapon)
	void SetRightHandWeapon(UTP_WeaponComponent* weapon);

	UFUNCTION(BlueprintCallable, Category = Weapon)
	bool GetHasLeftHandWeapon();

	UFUNCTION(BlueprintCallable, Category = Weapon)
	bool GetHasRightHandWeapon();

	UFUNCTION(BlueprintCallable, Category = Weapon)
	UTP_WeaponComponent* GetWeapon(EquipSlot SlotType);

	/** Getter for the weapon */
	UFUNCTION(BlueprintCallable, Category = Weapon)
	UTP_WeaponComponent* GetLeftHandWeapon();

	/** Getter for the weapon */
	UFUNCTION(BlueprintCallable, Category = Weapon)
	UTP_WeaponComponent* GetRightHandWeapon();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Inventory, meta = (AllowPrivateAccess = "true"))
	bool LeftHanded = false;

	FName GetTargetSocketName(EquipSlot SlotType);

	bool TrySetWeapon(UTP_WeaponComponent* Weapon, FName SocketName);

protected:
	/** Called for movement input */
	void Move(const FInputActionValue& Value);

	/** Called for looking input */
	void Look(const FInputActionValue& Value);

	void Jump();

	float slideElapsedSeconds;
	int launchesPerformed;


	// APawn interface
	virtual void SetupPlayerInputComponent(UInputComponent* InputComponent) override;
	void ToggleCrouch();
	void ToggleSprint();
	void LaunchJump();
	void TryUnCrouch();
	void EndSlide(UCharacterMovementComponent* movement);
	void SwitchCamera();
	// End of APawn interface

	void UseCapsuleForCollision(TObjectPtr<UCapsuleComponent> capsule);
};


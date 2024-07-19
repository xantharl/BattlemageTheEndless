// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "GameFramework/CharacterMovementComponent.h" 
#include <GameFramework/SpringArmComponent.h>
#include "../Pickups/TP_WeaponComponent.h"
#include "cmath"
#include <chrono>
#include "../Helpers/VectorMath.h"
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

	/** Wallrun overlap detection capsule **/
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Collision, meta = (AllowPrivateAccess = "true"))
	UCapsuleComponent* WallRunCapsule;

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
	milliseconds _lastCameraSwap;
	milliseconds _lastJumpTime;

public:
	ABattlemageTheEndlessCharacter();

	virtual void ApplyDamage(float damage);

protected:
	virtual void BeginPlay();
	virtual void OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode = 0);
	void SetupCameras();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Character)
	float Health = 100;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Character)
	float MaxHealth = 100;

	FTimerHandle DodgeEndTimer;

	FTimerHandle VaultEndTimer;

	float PreviousFriction = 8.0f;

	// Object currently being vaulted
	AActor* VaultTarget;
	FHitResult VaultHit;
	FVector VaultAttachPoint;

	// Object currently being wallrunned
	AActor* WallRunObject;
	FHitResult WallRunHit;
	FVector WallRunAttachPoint;
	FTimerHandle WallRunTimer;
	bool WallIsToLeft;

public:
		
	/** Look Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* LookAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CharacterMovement, meta = (AllowPrivateAccess = "true"))
	bool bIsSprinting;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CharacterMovement, meta = (AllowPrivateAccess = "true"))
	bool IsSliding;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CharacterMovement, meta = (AllowPrivateAccess = "true"))
	bool IsVaulting;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CharacterMovement, meta = (AllowPrivateAccess = "true"))
	bool IsWallRunning;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CharacterMovement, meta = (AllowPrivateAccess = "true"))
	bool bShouldUncrouch;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CharacterMovement, meta = (AllowPrivateAccess = "true"))
	bool bLaunchRequested;

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
	float DodgeDurationSeconds = 0.35f;

	// Default duration is based on the animation currently in use
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CharacterMovement, meta = (AllowPrivateAccess = "true"))
	float VaultDurationSeconds = 1.1f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CharacterMovement, meta = (AllowPrivateAccess = "true"))
	bool VaultFootPlanted = false;

	float VaultElapsedTimeBeforeFootPlanted = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CharacterMovement, meta = (AllowPrivateAccess = "true"))
	float VaultEndForwardDistance = 25.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CharacterMovement, meta = (AllowPrivateAccess = "true"))
	float CrouchSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CharacterMovement, meta = (AllowPrivateAccess = "true"))
	float LaunchSpeedHorizontal;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CharacterMovement, meta = (AllowPrivateAccess = "true"))
	float LaunchSpeedVertical;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CharacterMovement, meta = (AllowPrivateAccess = "true"))
	FVector DodgeImpulseLateral;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CharacterMovement, meta = (AllowPrivateAccess = "true"))
	FVector DodgeImpulseForward;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CharacterMovement, meta = (AllowPrivateAccess = "true"))
	FVector DodgeImpulseBackward;

	// Value expressed in m/s^2
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CharacterMovement, meta = (AllowPrivateAccess = "true"))
	float WallRunInitialGravitScale = 0.0f;

	// Value expressed in m/s^2
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CharacterMovement, meta = (AllowPrivateAccess = "true"))
	float CharacterBaseGravityScale = 1.75f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CharacterMovement, meta = (AllowPrivateAccess = "true"))
	float WallRunSpeed = 1000.f;

	/** Time before gravity starts to apply again **/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CharacterMovement, meta = (AllowPrivateAccess = "true"))
	float WallRunGravityDelay = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CharacterMovement, meta = (AllowPrivateAccess = "true"))
	float WallRunMaxDuration = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CharacterMovement, meta = (AllowPrivateAccess = "true"))
	int WallRunJumpRefundCount = 1;

	// TODO: Probably remove this since we can only launch out of crouch now
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CharacterMovement, meta = (AllowPrivateAccess = "true"))
	int MaxLaunches = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CharacterMovement, meta = (AllowPrivateAccess = "true"))
	double JumpCooldown = 0.05;

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
	void Crouch();
	void StartSprint();
	void EndSprint();
	void LaunchJump();
	void RequestUnCrouch();
	void TickActor(float DeltaTime, ELevelTick TickType, FActorTickFunction& ThisTickFunction);
	bool WallRunContinuationRayCast();
	void EndSlide(UCharacterMovementComponent* movement);
	void SwitchCamera();
	// Executes a launch jump 
	void DoLaunchJump();
	void DodgeInput();

	/// <summary>
	/// Executes a dodge in the given direction
	/// </summary>
	/// <param name="Impulse">The force with which to dodge (launch) the character, inclusive of direction</param>
	void Dodge(FVector Impulse);

	UFUNCTION(BlueprintCallable, Category = "Character Movement")
	void RestoreFriction();

	UFUNCTION(BlueprintCallable, Category = "Collision")
	void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);
	bool CanVault();
	bool ObjectIsVaultable(AActor* Object);
	void Vault();

	UFUNCTION(BlueprintCallable, Category = "Character Movement")
	void EndVault();

	UFUNCTION(BlueprintCallable, Category = "Character Movement")
	void OnWallRunCapsuleBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
	UFUNCTION(BlueprintCallable, Category = "Character Movement")
	void OnWallRunCapsuleEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
	bool CanWallRun();
	FHitResult LineTraceMovementVector(FName socketName, float magnitude, bool drawTrace, FColor drawColor, float rotateYawByDegrees);
	FHitResult LineTraceGeneric(FVector start, FVector end);
	bool ObjectIsWallRunnable(AActor* Object);
	void WallRun();

	UFUNCTION(BlueprintCallable, Category = "Character Movement")
	void EndWallRun();

	bool bCanVault = true;

	FVector lastTickLocation = FVector::ZeroVector;
};


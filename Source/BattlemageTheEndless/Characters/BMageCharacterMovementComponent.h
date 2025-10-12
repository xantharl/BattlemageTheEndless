// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameplayAbilities/Public/AbilitySystemComponent.h"
#include "Camera/CameraComponent.h"
#include <map>
#include <string>
#include <chrono>

#include "../Abilities/MovementAbility.h"
#include "../Abilities/Instances/WallRunAbility.h"
#include "../Abilities/Instances/VaultAbility.h"
#include "../Abilities/Instances/SlideAbility.h"
#include "../Abilities/Instances/LaunchAbility.h"
#include "../Abilities/Instances/DodgeAbility.h"
#include "../Abilities/Instances/SprintAbility.h"
#include "../Abilities/Instances/SlideAbility.h"

#include "../Helpers/VectorMath.h"
#include "../Helpers/Traces.h"

#include "BMageCharacterMovementComponent.generated.h"

using namespace std;
using namespace std::chrono;

/**
 * 
 */
UCLASS()
class BATTLEMAGETHEENDLESS_API UBMageCharacterMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()	

public:
	UBMageCharacterMovementComponent();

	UPROPERTY(EditAnywhere, Instanced, BlueprintReadWrite, Category = CharacterMovement, meta = (AllowPrivate))
	TMap<MovementAbilityType, TObjectPtr<UMovementAbility>> MovementAbilities;

	void InitAbilities(ACharacter* Character, USkeletalMeshComponent* Mesh);

	void ApplyInput();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CharacterMovement, meta = (AllowPrivateAccess = "true"))
	float BaseAirControl = 0.8f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CharacterMovement, meta = (AllowPrivateAccess = "true"))
	FVector PreviousVelocity;

	// Value expressed in m/s^2
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CharacterMovement, meta = (AllowPrivateAccess = "true"))
	float CharacterBaseGravityScale = 1.75f;

	// Value expressed in m/s^2
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CharacterMovement, meta = (AllowPrivateAccess = "true"))
	float CharacterPastJumpApexGravityScale = 2.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CharacterMovement, meta = (AllowPrivateAccess = "true"))
	int MaxLaunches = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CharacterMovement, meta = (AllowPrivateAccess = "true"))
	int LaunchesPerformed = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CharacterMovement, meta = (AllowPrivateAccess = "true"))
	float WalkSpeed = 500.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CharacterMovement, meta = (AllowPrivateAccess = "true"))
	float CrouchSpeed = 300.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CharacterMovement, meta = (AllowPrivateAccess = "true"))
	float ReverseSpeed = 300.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CharacterMovement, meta = (AllowPrivateAccess = "true"))
	float DefaultCrouchedHalfHeight = 65.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CharacterMovement, meta = (AllowPrivateAccess = "true"))
	bool ShouldUnCrouch = false;

	void RequestUnCrouch();

	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/// <summary>
	/// Tries to start an ability and returns whether it was successful
	/// </summary>
	/// <param name="abilityType"></param>
	/// <param name="Character"></param>
	/// <param name="mesh"></param>
	/// <returns></returns>
	UFUNCTION(BlueprintCallable, Category = CharacterMovement)
	UMovementAbility* TryStartAbility(MovementAbilityType abilityType);

	bool ShouldAbilityBegin(MovementAbilityType abilityType);

	/// <summary>
	/// Tries to end an ability and returns whether it was successful
	/// </summary>
	/// <param name="abilityType"></param>
	/// <param name="Character"></param>
	/// <param name="mesh"></param>
	/// <returns></returns>
	UFUNCTION(BlueprintCallable, Category = CharacterMovement)
	bool TryEndAbility(MovementAbilityType abilityType);

	/// <summary>
	/// Forces an ability to end if it is active
	/// </summary>
	/// <param name="abilityType"></param>
	UFUNCTION(BlueprintCallable, Category = CharacterMovement)
	void ForceEndAbility(MovementAbilityType abilityType);

	/// <summary>
	/// Checks whether the provided ability type is currently active
	/// </summary>
	/// <param name="abilityType"></param>
	/// <returns></returns>
	bool IsAbilityActive(MovementAbilityType abilityType) { return MovementAbilities.Contains(abilityType) && MovementAbilities[abilityType]->IsActive; }

	UPROPERTY(BlueprintReadOnly, Category = CharacterMovement)
	UMovementAbility* MostImportantActiveAbility;

	/// <summary>
	/// This is used for anim graph transitions
	/// </summary>
	/// <param name="abilityType"></param>
	/// <returns></returns>
	UFUNCTION(BlueprintCallable, Category = CharacterMovement)
	MovementAbilityType MostImportantActiveAbilityType();

	/// <summary>
	/// This is a hack to look through to wallrunability since i can't
	/// get the class UWallRunAbility to play nice in blueprint
	/// </summary>
	/// <returns></returns>
	UFUNCTION(BlueprintCallable, Category = CharacterMovement)
	bool IsWallRunToLeft();

	UFUNCTION(BlueprintCallable, Category = CharacterMovement)
	virtual void OnMovementAbilityBegin(UMovementAbility* MovementAbility);

	UFUNCTION(BlueprintCallable, Category = CharacterMovement)
	virtual void OnMovementAbilityEnd(UMovementAbility* ability);

	UFUNCTION(BlueprintCallable, Category = CharacterMovement)
	virtual void OnMovementAbilityShouldTransitionOut(UMovementAbility* ability);

	virtual void OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode) override;

	/// <summary>
	/// Exposes UVaultAbility::FootPlanted to blueprint for animnotify purposes
	/// </summary>
	/// <param name="value"></param>
	UFUNCTION(BlueprintCallable, Category = CharacterMovement)
	void SetVaultFootPlanted(bool value);

	/// <summary>
	/// Returns the sprint speed from the sprint ability
	/// </summary>
	/// <returns></returns>
	float SprintSpeed() const;

	/** Called when abilities which preserve gained speed wear off **/
	void ResetWalkSpeed();

	/** Apply gravity modification over time, duration is determined by the curve in MS **/
	UFUNCTION(BlueprintCallable, Category = Abilities)
	void BeginGravityOverTime(UCurveFloat* gravityCurve);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CharacterMovement, meta = (AllowPrivateAccess = "true"))
	bool ApplyMovementInputToJump = true;

	void HandleJump(UCameraComponent* activeCamera);

	void RedirectVelocityToLookDirection(bool wallrunEnded, UCameraComponent* activeCamera);

private:
	// Gravity over time operations
	void TickGravityOverTime(float DeltaTime);

	UCurveFloat* _activeGravityCurve;
	milliseconds _gravityCurveElapsed = milliseconds::zero();
	milliseconds _gravityCurveDuration = milliseconds::zero();
	float _initialGravityScale = 1.f;
};

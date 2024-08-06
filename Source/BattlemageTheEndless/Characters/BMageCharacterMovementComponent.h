// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include <map>
#include <string>

#include "../Abilities/MovementAbility.h"
#include "../Abilities/Instances/WallRunAbility.h"
#include "../Abilities/Instances/VaultAbility.h"
#include "../Abilities/Instances/SlideAbility.h"
#include "../Abilities/Instances/LaunchAbility.h"
#include "../Abilities/Instances/DodgeAbility.h"
//#include "../Abilities/Instances/DoubleJumpAbility.h"
#include "../Abilities/Instances/SprintAbility.h"
#include "../Abilities/Instances/SlideAbility.h"

#include "../Helpers/Traces.h"

#include "BMageCharacterMovementComponent.generated.h"

using namespace std;
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

	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/// <summary>
	/// Tries to start an ability and returns whether it was successful
	/// </summary>
	/// <param name="abilityType"></param>
	/// <param name="Character"></param>
	/// <param name="mesh"></param>
	/// <returns></returns>
	bool TryStartAbility(MovementAbilityType abilityType);

	/// <summary>
	/// Tries to end an ability and returns whether it was successful
	/// </summary>
	/// <param name="abilityType"></param>
	/// <param name="Character"></param>
	/// <param name="mesh"></param>
	/// <returns></returns>
	bool TryEndAbility(MovementAbilityType abilityType);

	/// <summary>
	/// Forces an ability to end if it is active
	/// </summary>
	/// <param name="abilityType"></param>
	void ForceEndAbility(MovementAbilityType abilityType);

	/// <summary>
	/// Checks whether the provided ability type is currently active
	/// </summary>
	/// <param name="abilityType"></param>
	/// <returns></returns>
	bool IsAbilityActive(MovementAbilityType abilityType) { return MovementAbilities[abilityType]->IsActive; }

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

	virtual void OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode) override;

	UFUNCTION(BlueprintCallable, Category = CharacterMovement)
	void SetVaultFootPlanted(bool value);
};

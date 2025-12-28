// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "MovementAbilityInstance.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include <chrono>
#include "GameFramework/CharacterMovementComponent.h"
#include "MovementAbility.generated.h"

UENUM(BlueprintType)
enum class MovementAbilityType: uint8
{
	None UMETA(DisplayName = "None"),
	Sprint UMETA(DisplayName = "Sprint"),
	Slide UMETA(DisplayName = "Slide"),
	Launch UMETA(DisplayName = "Launch"),
	WallRun UMETA(DisplayName = "WallRun"),
	Vault UMETA(DisplayName = "Vault"),
	Dodge UMETA(DisplayName = "Dodge")
};

using namespace std::chrono;

/**
 * This class is intended to be used as a base class for all movement abilities which have a ticking element
 */
DECLARE_DYNAMIC_MULTICAST_SPARSE_DELEGATE_OneParam(FMovementAbilityBeginSignature, UMovementAbility, OnMovementAbilityBegin, UMovementAbility*, MovementAbility);
DECLARE_DYNAMIC_MULTICAST_SPARSE_DELEGATE_OneParam(FMovementAbilityEndSignature, UMovementAbility, OnMovementAbilityEnd, UMovementAbility*, MovementAbility);
DECLARE_DYNAMIC_MULTICAST_SPARSE_DELEGATE_OneParam(FMovementAbilityShouldTransitionOutSignature, UMovementAbility, OnMovementAbilityShouldTransitionOut, UMovementAbility*, MovementAbility);
UCLASS(Abstract)
class BATTLEMAGETHEENDLESS_API UMovementAbility : public UGameplayAbility
{
	GENERATED_BODY()
protected:
	ACharacter* Character;
	USkeletalMeshComponent* Mesh;
	UCharacterMovementComponent* Movement;

	bool isTransitioningIn = false;
	bool shouldTransitionOut = false;

	milliseconds startTime;
	milliseconds elapsed;

	FTimerHandle transitionOutTimerHandle;

	//void onEndTransitionIn();
	void onEndTransitionOut();

public:
	UMovementAbility(const FObjectInitializer& X);
	//UMovementAbility() {}
	~UMovementAbility();

	// Determines whether a character can use this ability
	bool IsEnabled = true;

	// Determines whether the ability is currently active
	UFUNCTION(BlueprintCallable)
	bool IsGAActive();
	
	UFUNCTION(BlueprintCallable)
	bool ISMAActive(){return elapsed > static_cast<milliseconds>(0);}

	// Lower values are executed first
	int Priority = 10;

	UPROPERTY(BlueprintReadOnly)
	MovementAbilityType Type;

	virtual void Init(UCharacterMovementComponent* movement, ACharacter* character, USkeletalMeshComponent* mesh);
	virtual bool ShouldBegin() { return true; }
	virtual bool ShouldEnd() { return true; }
	virtual void Begin();
	virtual void End(bool bForce = false);
	// This is so we have a parameterless function for simple timer invocations
	virtual void OnEndTimer() { End(); }
	virtual void Tick(float DeltaTime);

	UFUNCTION(BlueprintCallable, Category = CharacterMovement, meta = (AllowPrivateAccess = "true"))
	void DeactivateAndBroadcast();

	/** Time till transition in animation is done **/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CharacterMovement, meta = (AllowPrivateAccess = "true"))
	float TransitionInDuration = 0.0f;

	/** Time till transition out animation is done **/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CharacterMovement, meta = (AllowPrivateAccess = "true"))
	float TransitionOutDuration = 0.0f;

	UFUNCTION(BlueprintCallable, Category = CharacterMovement, meta = (AllowPrivateAccess = "true"))
	virtual bool IsTransitioningIn() { return isTransitioningIn; }
	UFUNCTION(BlueprintCallable, Category = CharacterMovement, meta = (AllowPrivateAccess = "true"))
	virtual bool ShouldTransitionOut() { return shouldTransitionOut; }

	UPROPERTY(BlueprintAssignable, Category = "MovementAbility")
	FMovementAbilityBeginSignature OnMovementAbilityBegin;
	UPROPERTY(BlueprintAssignable, Category = "MovementAbility")
	FMovementAbilityEndSignature OnMovementAbilityEnd;
	UPROPERTY(BlueprintAssignable, Category = "MovementAbility")
	FMovementAbilityShouldTransitionOutSignature OnMovementAbilityShouldTransitionOut;
};
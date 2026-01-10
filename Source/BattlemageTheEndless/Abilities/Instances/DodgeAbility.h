// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "../MovementAbility.h"
#include "GameFramework/Character.h"
#include "DodgeAbility.generated.h"

/**
 * 
 */
UCLASS()
class BATTLEMAGETHEENDLESS_API UDodgeAbility : public UMovementAbility
{
	GENERATED_BODY()
	
public:
	FTimerHandle DodgeEndTimer;
	float PreviousFriction = 8.0f;

	UDodgeAbility(const FObjectInitializer& X);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CharacterMovement, meta = (AllowPrivateAccess = "true"))
	float DodgeDurationSeconds = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CharacterMovement, meta = (AllowPrivateAccess = "true"))
	FVector DodgeImpulseLateral = FVector(0.f, -1500.f, 0.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CharacterMovement, meta = (AllowPrivateAccess = "true"))
	FVector DodgeImpulseForward = FVector(1500.f, 0, 0);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CharacterMovement, meta = (AllowPrivateAccess = "true"))
	FVector DodgeImpulseBackward = FVector(-1000.f, 0, 500.f);
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CharacterMovement, meta = (AllowPrivateAccess = "true"))
	float ImpulseMultiplierAir = 0.5f;

	virtual void Begin(const FGameplayEventData* TriggerEventData = nullptr) override;
	virtual void End(bool bForce = false) override;
	
	virtual TObjectPtr<UObject> BuildMovementAbilityEventData() override;
	
	FVector LastInputVector = FVector::ZeroVector;
};

// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "../MovementAbility.h"
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
	TEnumAsByte<EMovementMode> PreviousMovementMode = MOVE_Walking;
	bool bChangedMovementMode = false;

	UDodgeAbility(const FObjectInitializer& X);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Dodge, meta = (AllowPrivateAccess = "true"))
	float DodgeDurationSeconds = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Dodge, meta = (AllowPrivateAccess = "true"))
	FVector DodgeImpulseLateral = FVector(0.f, -1500.f, 0.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Dodge, meta = (AllowPrivateAccess = "true"))
	FVector DodgeImpulseForward = FVector(1500.f, 0, 0);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Dodge, meta = (AllowPrivateAccess = "true"))
	FVector DodgeImpulseBackward = FVector(-1000.f, 0, 500.f);
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Dodge, meta = (AllowPrivateAccess = "true"))
	float ImpulseMultiplierAir = 0.5f;

	virtual void Begin(const FMovementEventData& MovementEventData) override;
	virtual void End(bool bForce = false) override;
	virtual FMovementEventData BuildMovementEventData() const override;

	UPROPERTY(BlueprintReadOnly, Category = Dodge)
	FVector LastInputVector = FVector::ZeroVector;

protected:
	// Returns the dodge impulse in pawn-local space (X=forward, Y=right) by angularly
	// interpolating between the two cardinal impulses adjacent to the input direction.
	FVector ComputeDodgeImpulse(const FVector& LocalInput) const;
};

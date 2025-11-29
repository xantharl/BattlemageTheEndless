// Fill out your copyright notice in the Description page of Project Settings.


#include "DodgeAbility.h"

UDodgeAbility::UDodgeAbility(const FObjectInitializer& X) : Super(X)
{
	Type = MovementAbilityType::Dodge;
	// Make this ability override sprint
	Priority = 2;
}

void UDodgeAbility::Begin()
{
	FVector inputVector = Character->GetLastMovementInputVector();
	// account for camera rotation
	inputVector = inputVector.RotateAngleAxis(Movement->GetLastUpdateRotation().GetInverse().Yaw, FVector::ZAxisVector);

	// use the strongest input direction to decide which way to dodge
	bool isLateral = FMath::Abs(inputVector.X) < FMath::Abs(inputVector.Y);

	FVector ImpulseToUse;
	// if the movement is not lateral, dodge forward or backward
	if (!isLateral)
		ImpulseToUse = inputVector.X > 0.0f ? DodgeImpulseForward : DodgeImpulseBackward;
	// if the movement is lateral, dodge left or right
	else
		ImpulseToUse = DodgeImpulseLateral * ((inputVector.Y < 0.0f) ? 1.f : -1.f);
	
	// modify impulse if in air
	if (Movement->MovementMode == EMovementMode::MOVE_Falling)
	{
		ImpulseToUse *= ImpulseMultiplierAir;
	}

	// Launch the character
	PreviousFriction = Movement->GroundFriction;
	Movement->GroundFriction = 0.0f;
	FVector dodgeVector = FVector(ImpulseToUse.RotateAngleAxis(Movement->GetLastUpdateRotation().Yaw, FVector::ZAxisVector));
	Character->LaunchCharacter(dodgeVector, true, true);
	Character->GetWorldTimerManager().SetTimer(DodgeEndTimer, this, &UDodgeAbility::OnEndTimer, DodgeDurationSeconds, false);

	Super::Begin();
}

void UDodgeAbility::End(bool bForce)
{
	Super::End(bForce);
	Movement->GroundFriction = PreviousFriction;
}

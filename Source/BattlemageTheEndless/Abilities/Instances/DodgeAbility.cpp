// Fill out your copyright notice in the Description page of Project Settings.

#include "DodgeAbility.h"

UDodgeAbility::UDodgeAbility(const FObjectInitializer& X) : Super(X)
{
	Type = MovementAbilityType::Dodge;
	// Make this ability override sprint
	Priority = 2;
}

void UDodgeAbility::Begin(const FMovementEventData& MovementEventData)
{
	FVector InputVector;
	// If MovementEventData was not provided, we are using the local path and can rely on local vars
	LastInputVector = MovementEventData.OptionalVector == FVector::ZeroVector ? Character->GetLastMovementInputVector() : MovementEventData.OptionalVector;
	PreviousFriction = MovementEventData.OptionalFloat == 0.f ? Movement->GroundFriction : MovementEventData.OptionalFloat;
	if (LastInputVector != FVector::ZeroVector)
	{
		InputVector = LastInputVector;
		// account for camera rotation
		InputVector = InputVector.RotateAngleAxis(Movement->GetLastUpdateRotation().GetInverse().Yaw, FVector::ZAxisVector);
	}
	else
	{
		InputVector = Character->GetLastMovementInputVector();
		// account for camera rotation
		InputVector = InputVector.RotateAngleAxis(Movement->GetLastUpdateRotation().GetInverse().Yaw, FVector::ZAxisVector);
	}
	
	FVector DodgeVector = FVector::ZeroVector;
	// Forward / back contribution
	if (InputVector.X >= 0.f) {
		DodgeVector += DodgeImpulseForward * InputVector.X;
	} else {
		DodgeVector += DodgeImpulseBackward * -InputVector.X;
	}

	// Lateral contribution (right is positive, left is negative)
	DodgeVector.Y += DodgeImpulseLateral.Y * -InputVector.Y;             // signed
	DodgeVector.X += DodgeImpulseLateral.X * FMath::Abs(InputVector.Y); // unsigned
	DodgeVector.Z += DodgeImpulseLateral.Z * FMath::Abs(InputVector.Y); // unsigned

	// Launch the character
	Movement->GroundFriction = 0.0f;
	FVector WorldImpulse = Character->GetActorTransform().TransformVectorNoScale(DodgeVector);

	// modify impulse if in air
	if (Movement->MovementMode == EMovementMode::MOVE_Falling)
	{
		float LookPitch = FRotator::NormalizeAxis(Character->GetControlRotation().Pitch);
		WorldImpulse = WorldImpulse.RotateAngleAxis(-LookPitch, Character->GetActorRightVector());
		WorldImpulse *= ImpulseMultiplierAir;
	}
	
	Character->LaunchCharacter(WorldImpulse, true, true);
	Character->GetWorldTimerManager().SetTimer(DodgeEndTimer, this, &UDodgeAbility::OnEndTimer, DodgeDurationSeconds, false);

	Super::Begin(MovementEventData);
}

void UDodgeAbility::End(bool bForce)
{
	Super::End(bForce);
	Movement->GroundFriction = PreviousFriction;
}

FMovementEventData UDodgeAbility::BuildMovementEventData() const
{
	auto PawnMovement = Character->GetMovementComponent();
	if (!PawnMovement)
	{
		UE_LOG(LogTemp, Warning, TEXT( "UMovementAbility::BuildMovementEventDataFromPawn - Pawn %s has no "
						   "MovementComponent, cannot build MovementEventData" ), *Character->GetName() );
		return FMovementEventData();
	}
	
	FMovementEventData ReturnData = FMovementEventData();
	ReturnData.OptionalVector = PawnMovement->GetLastInputVector();						
	if (IsValid(Movement))
		ReturnData.OptionalFloat = Movement->GroundFriction;
	else
		UE_LOG(LogTemp, Warning, TEXT( "UMovementAbility::BuildMovementEventDataFromPawn - Character %s has no "
						   "CharacterMovementComponent, cannot populate MovementEventData.OptionalFloat" ), *GetName() );
	return ReturnData;
}

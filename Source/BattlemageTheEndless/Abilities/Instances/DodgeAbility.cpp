// Fill out your copyright notice in the Description page of Project Settings.


#include "DodgeAbility.h"
#include "BattlemageTheEndless/Abilities/EventData/DodgeEventData.h"

UDodgeAbility::UDodgeAbility(const FObjectInitializer& X) : Super(X)
{
	Type = MovementAbilityType::Dodge;
	// Make this ability override sprint
	Priority = 2;
}

void UDodgeAbility::Begin(const FGameplayEventData* TriggerEventData)
{
	FVector InputVector;
	// Server path which takes input from event data provided by client
	if (TriggerEventData && TriggerEventData->OptionalObject && TriggerEventData->OptionalObject->IsA(UDodgeEventData::StaticClass()))
	{
		const UDodgeEventData* DodgeData = Cast<UDodgeEventData>(TriggerEventData->OptionalObject);
		InputVector = DodgeData->DodgeInputVector;
	}
	else
	{
		InputVector = Character->GetLastMovementInputVector();
		// account for camera rotation
		InputVector = InputVector.RotateAngleAxis(Movement->GetLastUpdateRotation().GetInverse().Yaw, FVector::ZAxisVector);
	}

	// use the strongest input direction to decide which way to dodge
	bool isLateral = FMath::Abs(InputVector.X) < FMath::Abs(InputVector.Y);

	FVector ImpulseToUse;
	// if the movement is not lateral, dodge forward or backward
	if (!isLateral)
		ImpulseToUse = InputVector.X > 0.0f ? DodgeImpulseForward : DodgeImpulseBackward;
	// if the movement is lateral, dodge left or right
	else
		ImpulseToUse = DodgeImpulseLateral * ((InputVector.Y < 0.0f) ? 1.f : -1.f);
	
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

TObjectPtr<UObject> UDodgeAbility::BuildMovementAbilityEventData()
{
	UDodgeEventData* DodgeData = NewObject<UDodgeEventData>();
	DodgeData->DodgeInputVector = Character->GetLastMovementInputVector();
	return DodgeData;
}

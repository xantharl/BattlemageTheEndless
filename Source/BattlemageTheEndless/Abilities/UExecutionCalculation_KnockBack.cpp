// Fill out your copyright notice in the Description page of Project Settings.


#include "UExecutionCalculation_KnockBack.h"

void UUExecutionCalculation_KnockBack::Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, OUT FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
	// Get a reference to the target ASC's owner
	AActor* TargetActor = ExecutionParams.GetTargetAbilitySystemComponent()->GetOwner();
	if (!TargetActor)
	{
		return;
	}

	ACharacter* TargetCharacter = Cast<ACharacter>(TargetActor);
	if (!TargetCharacter)
	{
		return;
	}

	// If the caster has been killed, this will be a null pointer
	if (!ExecutionParams.GetSourceAbilitySystemComponent())
	{
		return;
	}
	// Use the Source's forward vector to determine the direction of the knockback	
	AActor* SourceActor = ExecutionParams.GetSourceAbilitySystemComponent()->GetOwner();

	// if this was a self cast ability, use the character's current rotation instead
	FVector transformedKnockback = KnockBackVelocity;

	if (!SourceActor)
	{
		UE_LOG(LogTemp, Error, TEXT("UExecutionCalculation_KnockBack: Missing Source Actor"));
		return;
	}

	if (SourceActor == TargetActor)
	{
		transformedKnockback = transformedKnockback.RotateAngleAxis(TargetActor->GetActorRotation().Yaw, FVector::ZAxisVector);
	}
	else
	{
		FVector Direction = TargetCharacter->GetActorLocation() - SourceActor->GetActorLocation();
		Direction.Normalize();
		if (!ConsiderZAxis) // this is for abilities which are placed on the ground
		{
			Direction.Z = 0; 
		}
		else
		{
			// attempt to get the camera's pitch, requires that sourceActor is a BattlemageTheEndlessCharacter
			auto BMageCharacter = Cast<ABattlemageTheEndlessCharacter>(SourceActor);
			if (BMageCharacter)
			{
				FVector cameraRotation = BMageCharacter->GetActiveCamera()->GetComponentRotation().Vector();
				Direction.Z = cameraRotation.Z;
			}
		}
		transformedKnockback = Direction.Rotation().RotateVector(KnockBackVelocity);

		// If the target character is currently on the ground and we have negative Z, 0 it out
		auto targetMovement = TargetCharacter->GetMovementComponent();
		if (targetMovement && !targetMovement->IsFalling() && transformedKnockback.Z < 0)
		{ 
			transformedKnockback.Z = 0;
		}
	}

	TargetCharacter->LaunchCharacter(transformedKnockback, true, true);

	// check if target character's movement component is valid and is a BMageCharacterMovementComponent
	if (GravityScaleOverTime)
	{
		UBMageCharacterMovementComponent* MovementComponent = Cast<UBMageCharacterMovementComponent>(TargetCharacter->GetCharacterMovement());
		if (MovementComponent)
			MovementComponent->BeginGravityOverTime(GravityScaleOverTime);
	}
}

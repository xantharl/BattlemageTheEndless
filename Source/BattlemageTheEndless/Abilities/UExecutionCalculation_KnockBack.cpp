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
		Direction.Z = 0; // just calculating horizontal direction right now
		Direction.Normalize();
		transformedKnockback = KnockBackVelocity.RotateAngleAxis(Direction.Rotation().Yaw, FVector::ZAxisVector);
	}

	TargetCharacter->LaunchCharacter(transformedKnockback, true, true);
}

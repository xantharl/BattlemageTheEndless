// Fill out your copyright notice in the Description page of Project Settings.


#include "ExecutionCalculation_LaunchAt.h"

UExecutionCalculation_LaunchAt::UExecutionCalculation_LaunchAt()
{
}

void UExecutionCalculation_LaunchAt::Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, OUT FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
	ABattlemageTheEndlessCharacter* sourceCharacter = Cast<ABattlemageTheEndlessCharacter>(ExecutionParams.GetSourceAbilitySystemComponent()->GetOwner());
	if (!sourceCharacter)
	{
		UE_LOG(LogTemp, Error, TEXT("ExecutionCalculation_LaunchAt: Invalid Source Character"));
		return;
	}

	TArray<AActor*> AllActors;
	UGameplayStatics::GetAllActorsOfClass(sourceCharacter, ACharacter::StaticClass(), AllActors);

	// for all characters
		// continue if equal to sourceCharacter
		// get their ability system component if they have one, if not continue
		// check if they are marked, if not continue
		// calculate distance between the two
		// check if they are within range of sourceCharacter, if not continue
		// calculate difference in source character look direction and vector between source and marked, this needs to consider yaw and pitch (which requires a look direction reference)
		// if they are marked and source character is facing them (sourceCharacter rotation is within TargetingToleranceDeg degrees of vector between source and marked)
			// if the distance is less than the current minimum distance
				// set current minimum distance to this distance
				// set current target to this character

	AActor* currentTarget = nullptr;
	float currentMinDistance = LaunchRange;
	for (AActor* actor : AllActors)
	{
		if (actor == sourceCharacter)
			continue;

		auto targetAsc = actor->FindComponentByClass<UAbilitySystemComponent>();
		if (!targetAsc)
			continue;

		if (!targetAsc->HasMatchingGameplayTag(FGameplayTag::RequestGameplayTag("GameplayCue.Status.Marked")))
			continue;

		float distance = FVector::Dist(actor->GetActorLocation(), sourceCharacter->GetActorLocation());
		if (distance > LaunchRange)
			continue;

		FVector directionToTarget = (actor->GetActorLocation() - sourceCharacter->GetActorLocation()).GetSafeNormal();

		FVector sourceForward = sourceCharacter->GetActiveCamera()->GetComponentRotation().Vector().GetSafeNormal();
		float angleBetween = FMath::Acos(FVector::DotProduct(directionToTarget, sourceForward)) * (180.f / PI);
		if (angleBetween > TargetingToleranceDeg)
		{
			continue;
		}
		if (distance < currentMinDistance)
		{
			currentMinDistance = distance;
			currentTarget = actor;
		}
	}

	if (currentTarget)
	{
		FVector launchDirection = (currentTarget->GetActorLocation() - sourceCharacter->GetActorLocation()).GetSafeNormal();
		sourceCharacter->LaunchCharacter(launchDirection * LaunchSpeed, true, true);

		// TODO: Predict target's location upon arrival? Currently just launches towards their current location
		//		Or should we just launch in aim direction so it's a skill based check?
	}
}

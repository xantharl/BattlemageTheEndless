// Fill out your copyright notice in the Description page of Project Settings.

#include "AIControllerSmoothFocus.h"
#include "BattlemageTheEndlessCharacter.h"
#include "SwarmManagerComponent.h"
#include "BehaviorTree/BlackboardComponent.h"

void AAIControllerSmoothFocus::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (!InPawn) return;

	USwarmManagerComponent* Swarm = USwarmManagerComponent::Get(this);
	if (!Swarm) return;

	Swarm->RegisterEnemy(InPawn);
	Swarm->HighestAggroChanged.AddDynamic(this, &AAIControllerSmoothFocus::OnHighestAggroChanged);
}

void AAIControllerSmoothFocus::OnUnPossess()
{
	APawn* CurrentPawn = GetPawn();
	if (!CurrentPawn)
	{
		Super::OnUnPossess();
		return;
	}

	USwarmManagerComponent* Swarm = USwarmManagerComponent::Get(this);
	if (Swarm)
	{
		Swarm->HighestAggroChanged.RemoveDynamic(this, &AAIControllerSmoothFocus::OnHighestAggroChanged);
		Swarm->UnregisterEnemy(CurrentPawn);
	}

	Super::OnUnPossess();
}

void AAIControllerSmoothFocus::OnHighestAggroChanged(AActor* Enemy, AActor* NewHighestThreat)
{
	if (Enemy != GetPawn()) return;

	USwarmManagerComponent* Swarm = USwarmManagerComponent::Get(this);
	if (!Swarm) return;

	if (NewHighestThreat)
	{
		Swarm->SwitchTargets(NewHighestThreat, GetPawn());
		return;
	}

	// No threat — clear the blackboard target and free our slot.
	if (UBlackboardComponent* BB = GetBlackboardComponent())
	{
		static const FName TargetActorKey(TEXT("TargetActor"));
		BB->ClearValue(TargetActorKey);
	}
	Swarm->FreeClaimedPosition(GetPawn());
}

void AAIControllerSmoothFocus::UpdateControlRotation(float DeltaTime, bool bUpdatePawn)
{
	if (auto* Char = Cast<ABattlemageTheEndlessCharacter>(GetPawn());
		Char && Char->IsDead)
	{
		return;
	}

	//Call the original method without updating the pawn
	Super::UpdateControlRotation(DeltaTime, false);

	//Smooth and change the pawn rotation
	if (bUpdatePawn)
	{
		//Get pawn
		APawn* const MyPawn = GetPawn();
		//Get Pawn current rotation
		const FRotator CurrentPawnRotation = MyPawn->GetActorRotation();
	 
		//Calculate smoothed rotation
		SmoothTargetRotation = UKismetMathLibrary::RInterpTo_Constant(MyPawn->GetActorRotation(), ControlRotation, DeltaTime, SmoothFocusInterpSpeed);
		//Check if we need to change
		if (CurrentPawnRotation.Equals(SmoothTargetRotation, 1e-3f) == false)
		{
			//Change rotation using the Smooth Target Rotation
			MyPawn->FaceRotation(SmoothTargetRotation, DeltaTime);
		}
	}
}

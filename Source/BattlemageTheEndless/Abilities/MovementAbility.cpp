// Fill out your copyright notice in the Description page of Project Settings.


#include "MovementAbility.h"

void UMovementAbility::onEndTransitionIn()
{
	isTransitioningIn = false;
}

void UMovementAbility::onEndTransitionOut()
{
}

UMovementAbility::UMovementAbility(const FObjectInitializer& X) : UObject(X)
{
}

UMovementAbility::~UMovementAbility()
{
}

void UMovementAbility::Init(UCharacterMovementComponent* movement, ACharacter* character, USkeletalMeshComponent* mesh)
{
	Movement = movement; 
	Character = character; 
	Mesh = mesh;
}

void UMovementAbility::Begin()
{
	IsActive = true;
	startTime = duration_cast<milliseconds>(system_clock::now().time_since_epoch());
	if (TransitionInDuration > 0.00001f)
		isTransitioningIn = true;

	OnMovementAbilityBegin.Broadcast(this);
}

void UMovementAbility::End()
{
	// If we need to transition out, inform the blueprint with the bool and set a timer to end the ability
	if (TransitionOutDuration > 0.00001f)
	{
		shouldTransitionOut = true;
		auto endLambda = [this]() {
			this->IsActive = false;
			this->OnMovementAbilityEnd.Broadcast(this);
			};
		GetWorld()->GetTimerManager().SetTimer(transitionOutTimerHandle, endLambda, TransitionOutDuration, false);
		return;
	}

	// Otherwise just end it now
	IsActive = false;
	OnMovementAbilityEnd.Broadcast(this);
}

void UMovementAbility::Tick(float DeltaTime)
{
	elapsed += round<milliseconds>(duration<float>{DeltaTime*1000.f});
	if (isTransitioningIn && elapsed > round<milliseconds>(duration<float>{TransitionInDuration * 1000.f}))
		isTransitioningIn = false;

}

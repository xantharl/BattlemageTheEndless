// Fill out your copyright notice in the Description page of Project Settings.


#include "MovementAbility.h"

#include "AbilitySystemComponent.h"
#include "GameFramework/Character.h"

//void UMovementAbility::onEndTransitionIn()
//{
//	isTransitioningIn = false;
//}

void UMovementAbility::onEndTransitionOut()
{
}

UMovementAbility::UMovementAbility(const FObjectInitializer& X) : UGameplayAbility(X)
{
}

UMovementAbility::~UMovementAbility()
{
}

bool UMovementAbility::IsGAActive()
{
	if (!Character)
	{
		UE_LOG(LogTemp, Error, TEXT("Character is NULL"));
		return false;
	}
	if (auto ASC = Character->GetComponentByClass<UAbilitySystemComponent>())
	{
		TArray<FGameplayAbilitySpecHandle> OutAbilityHandles;
		FString EnumName = StaticEnum<MovementAbilityType>()->GetNameStringByValue(static_cast<int64>(Type));
		FString LookupTagName = FString::Printf(TEXT("Movement.%s"), *EnumName);
		
		// special case because I didn't name my tag and enum consistently :(
		if (LookupTagName == "Movement.Launch")
		{
			LookupTagName = "Movement.LaunchJump";
		}
		
		ASC->FindAllAbilitiesWithTags(OutAbilityHandles, FGameplayTagContainer(FGameplayTag::RequestGameplayTag(FName(LookupTagName))));
		for (const auto Handle : OutAbilityHandles)
		{
			auto const Spec = ASC->FindAbilitySpecFromHandle(Handle);
			if (Spec && Spec->GetPrimaryInstance() && Spec->GetPrimaryInstance()->IsActive())
				return true;
		}
	}
	return false;
}

void UMovementAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
}

void UMovementAbility::Init(UCharacterMovementComponent* movement, ACharacter* character, USkeletalMeshComponent* mesh)
{
	Movement = movement; 
	Character = character; 
	Mesh = mesh;
}

void UMovementAbility::Begin(const FMovementEventData& MovementEventData)
{
	elapsed = (milliseconds)0;
	shouldTransitionOut = false;
	startTime = duration_cast<milliseconds>(system_clock::now().time_since_epoch());
	if (TransitionInDuration > 0.00001f)
	{
		// if (GEngine)
		// 	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Transitioning in"));
		isTransitioningIn = true;
	}

	OnMovementAbilityBegin.Broadcast(this);
}

void UMovementAbility::End(bool bForce)
{
	// If we're already transitioning out, don't do anything
	if (shouldTransitionOut)
		return;

	// If we need to transition out, inform the blueprint with the bool and set a timer to end the ability
	if (TransitionOutDuration > 0.00001f)
	{
		shouldTransitionOut = true;
		OnMovementAbilityShouldTransitionOut.Broadcast(this);
		FTimerDelegate TimerDelegate = FTimerDelegate::CreateUObject(this, &UMovementAbility::DeactivateAndBroadcast);
		GetWorld()->GetTimerManager().SetTimer(transitionOutTimerHandle, TimerDelegate, TransitionOutDuration, false);
		return;
	}

	// Otherwise just end it now
	DeactivateAndBroadcast();
}

void UMovementAbility::Tick(float DeltaTime)
{
	elapsed += round<milliseconds>(duration<float>{DeltaTime*1000.f});
	if (isTransitioningIn && elapsed > round<milliseconds>(duration<float>{TransitionInDuration * 1000.f}))
		isTransitioningIn = false;

}

void UMovementAbility::DeactivateAndBroadcast() {
	shouldTransitionOut = false;
	OnMovementAbilityEnd.Broadcast(this);
}

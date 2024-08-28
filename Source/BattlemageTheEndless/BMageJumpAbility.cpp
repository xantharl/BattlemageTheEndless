// Fill out your copyright notice in the Description page of Project Settings.


#include "BMageJumpAbility.h"
#include "GameFramework/Character.h"
#include "AbilitySystemComponent.h"

void UBMageJumpAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	// valiate character reference	
	if (ACharacter* Character = Cast<ACharacter>(ActorInfo->AvatarActor.Get()))
	{
		// Make the character jump
		Character->Jump();
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
}

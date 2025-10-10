// Fill out your copyright notice in the Description page of Project Settings.


#include "EC_StatusEffectElevated.h"

void UEC_StatusEffectElevated::Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, OUT FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
	// Check if the current effect 
	FGameplayTagContainer grantedTags;
	ExecutionParams.GetOwningSpec().GetAllGrantedTags(grantedTags);

}

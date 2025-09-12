// Fill out your copyright notice in the Description page of Project Settings.


#include "ExecutionCalculation_Marked.h"
UExecutionCalculation_Marked::UExecutionCalculation_Marked()
{
	ValidTransientAggregatorIdentifiers.AddTag(FGameplayTag::RequestGameplayTag("Data.Mark.Range"));
	ValidTransientAggregatorIdentifiers.AddTag(FGameplayTag::RequestGameplayTag("Data.Mark.Duration"));
}

void UExecutionCalculation_Marked::Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, OUT FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
	// Get references to target and source actors
	auto TargetAsc = Cast<UBMageAbilitySystemComponent>(ExecutionParams.GetTargetAbilitySystemComponent());
	AActor* SourceActor = ExecutionParams.GetSourceAbilitySystemComponent()->GetOwner();
	if (!SourceActor || !TargetAsc)
	{
		UE_LOG(LogTemp, Error, TEXT("ExecutionCalculation_Marked: Invalid Source or Target Actor"));
		return;
	}
	// Don't mark yourself
	if (SourceActor == TargetAsc->GetOwner())
	{
		return;
	}

	float range = 0.0f;
	ExecutionParams.AttemptCalculateTransientAggregatorMagnitude(FGameplayTag::RequestGameplayTag("Data.Mark.Range"), FAggregatorEvaluateParameters(), range);
	float duration = 0.0f;
	ExecutionParams.AttemptCalculateTransientAggregatorMagnitude(FGameplayTag::RequestGameplayTag("Data.Mark.Duration"), FAggregatorEvaluateParameters(), duration);

	TargetAsc->MarkOwner(SourceActor, duration, range, MarkedMaterial, MarkedMaterialOutOfRange);
}

// Fill out your copyright notice in the Description page of Project Settings.


#include "ExecutionCalculation_OnHitEffect.h"

void UExecutionCalculation_OnHitEffect::Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, OUT FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
	//Super::Execute_Implementation(ExecutionParams, OutExecutionOutput);

	if (EffectsToApply.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("No EffectsToApply set on OnHitEffect ExecutionCalculation"));
		return;
	}

	const FGameplayEffectSpec& Spec = ExecutionParams.GetOwningSpec();
	UAbilitySystemComponent* SourceAbilitySystem = ExecutionParams.GetSourceAbilitySystemComponent();
	UAbilitySystemComponent* TargetAbilitySystem = ExecutionParams.GetTargetAbilitySystemComponent();
	if (!TargetAbilitySystem)
	{
		UE_LOG(LogTemp, Warning, TEXT("No TargetAbilitySystemComponent found in OnHitEffect ExecutionCalculation"));
		return;
	}

	if (!SourceAbilitySystem)
	{
		UE_LOG(LogTemp, Warning, TEXT("No SourceAbilitySystemComponent found in OnHitEffect ExecutionCalculation"));
		return;
	}

	FGameplayTagContainer SourceTags;
	FGameplayTagContainer TargetTags;
	SourceAbilitySystem->GetOwnedGameplayTags(SourceTags);
	TargetAbilitySystem->GetOwnedGameplayTags(TargetTags);

	if (!HasRequiredAndNoBlockingTags(SourceTags, RequiredTagsSource, BlockingTagsSource) || !HasRequiredAndNoBlockingTags(TargetTags, RequiredTagsTarget, BlockingTagsTarget))
	{
		return;
	}

	for (TSubclassOf<UGameplayEffect> EffectClass : EffectsToApply)
	{
		if (!EffectClass)
		{
			UE_LOG(LogTemp, Warning, TEXT("Null EffectClass found in EffectsToApply of OnHitEffect ExecutionCalculation"));
			continue;
		}

		FGameplayEffectSpecHandle NewHandle = TargetAbilitySystem->MakeOutgoingSpec(EffectClass, Spec.GetLevel(), Spec.GetContext());
		if (NewHandle.IsValid())
		{
			TargetAbilitySystem->ApplyGameplayEffectSpecToSelf(*NewHandle.Data.Get());
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Failed to create GameplayEffectSpec from EffectClass %s in OnHitEffect ExecutionCalculation"), *EffectClass->GetName());
		}
	}
}

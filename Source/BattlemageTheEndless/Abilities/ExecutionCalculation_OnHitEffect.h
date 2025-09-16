 // Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectExecutionCalculation.h"
#include "GameplayAbilities/Public/AbilitySystemComponent.h"
#include "ExecutionCalculation_OnHitEffect.generated.h"

/**
 * This Execution Calculation is used to apply effects when a hit is confirmed, the intended use is by weapons.
 */
UCLASS()
class BATTLEMAGETHEENDLESS_API UExecutionCalculation_OnHitEffect : public UGameplayEffectExecutionCalculation
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "OnHitEffect")
	TArray<TSubclassOf<UGameplayEffect>> EffectsToApply;

	UPROPERTY(EditAnywhere, Category = "OnHitEffect")
	FGameplayTagContainer RequiredTagsSource;

	UPROPERTY(EditAnywhere, Category = "OnHitEffect")
	FGameplayTagContainer BlockingTagsSource;

	UPROPERTY(EditAnywhere, Category = "OnHitEffect")
	FGameplayTagContainer RequiredTagsTarget;

	UPROPERTY(EditAnywhere, Category = "OnHitEffect")
	FGameplayTagContainer BlockingTagsTarget;

	virtual void Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, OUT FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const override;

private:
	bool HasRequiredAndNoBlockingTags(const FGameplayTagContainer& OwnedTags, const FGameplayTagContainer& RequiredTags, const FGameplayTagContainer& BlockingTags) const
	{
		return OwnedTags.HasAll(RequiredTags) && !OwnedTags.HasAny(BlockingTags);
	}
};

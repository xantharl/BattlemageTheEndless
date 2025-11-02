// Fill out your copyright notice in the Description page of Project Settings.


#include "GE_SuspendRegenHealth.h"

UGE_SuspendRegenHealth::UGE_SuspendRegenHealth()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	auto setByCallerDuration = FSetByCallerFloat();
	setByCallerDuration.DataTag = FGameplayTag::RequestGameplayTag(FName("Attributes.SuspendRegenDuration"));
	DurationMagnitude = FGameplayEffectModifierMagnitude(setByCallerDuration);
	Modifiers.Add(FGameplayModifierInfo());
	Modifiers[0].ModifierOp = EGameplayModOp::Override;
	Modifiers[0].Attribute = FGameplayAttribute(UBaseAttributeSet::GetHealthRegenRateAttribute());

	auto setByCaller = FSetByCallerFloat();
	setByCaller.DataTag = FGameplayTag::RequestGameplayTag(FName("Attributes.HealthToRegen"));
	Modifiers[0].ModifierMagnitude = FGameplayEffectModifierMagnitude(setByCaller);

	StackingType = EGameplayEffectStackingType::AggregateBySource;
	StackLimitCount = 1;
	StackDurationRefreshPolicy = EGameplayEffectStackingDurationPolicy::RefreshOnSuccessfulApplication;
}

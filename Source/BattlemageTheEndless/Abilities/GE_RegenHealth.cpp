// Fill out your copyright notice in the Description page of Project Settings.


#include "GE_RegenHealth.h"

UGE_RegenHealth::UGE_RegenHealth()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;
	DurationMagnitude = FGameplayEffectModifierMagnitude(0.0f);
	Modifiers.Add(FGameplayModifierInfo());
	Modifiers[0].ModifierOp = EGameplayModOp::Additive;
	Modifiers[0].Attribute = FGameplayAttribute(UBaseAttributeSet::GetHealthAttribute());

	auto setByCaller = FSetByCallerFloat();
	setByCaller.DataTag = FGameplayTag::RequestGameplayTag(FName("Attributes.HealthToRegen"));
	Modifiers[0].ModifierMagnitude = FGameplayEffectModifierMagnitude(setByCaller);
}

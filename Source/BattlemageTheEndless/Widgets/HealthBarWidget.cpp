// Fill out your copyright notice in the Description page of Project Settings.


#include "HealthBarWidget.h"

void UHealthBarWidget::SubscribeToStackChanges(UAbilitySystemComponent* abilitySystemComponent, UBaseAttributeSet* attributeSet)
{
	_abilitySystemComponent = abilitySystemComponent;
	_attributeSet = attributeSet;

	_abilitySystemComponent->OnActiveGameplayEffectAddedDelegateToSelf.AddUObject(this, &UHealthBarWidget::OnActiveGameplayEffectAddedCallback);
	_abilitySystemComponent->OnAnyGameplayEffectRemovedDelegate().AddUObject(this, &UHealthBarWidget::OnRemoveGameplayEffectCallback);
	_abilitySystemComponent->GetGameplayAttributeValueChangeDelegate(attributeSet->GetHealthAttribute()).AddUObject(this, &UHealthBarWidget::HealthChanged);
}

void UHealthBarWidget::OnActiveGameplayEffectAddedCallback(UAbilitySystemComponent* Target, const FGameplayEffectSpec& SpecApplied, FActiveGameplayEffectHandle ActiveHandle)
{
}

void UHealthBarWidget::OnRemoveGameplayEffectCallback(const FActiveGameplayEffect& EffectRemoved)
{
}

void UHealthBarWidget::HealthChanged(const FOnAttributeChangeData& Data)
{
}

// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "AbilitySystemComponent.h"
#include "../Abilities/BaseAttributeSet.h"
#include "Components/RichTextBlockImageDecorator.h"
#include "HealthBarWidget.generated.h"

/**
 * 
 */
UCLASS()
class BATTLEMAGETHEENDLESS_API UHealthBarWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SubscribeToStackChanges(UAbilitySystemComponent* abilitySystemComponent, UBaseAttributeSet* attributeSet);

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Health Bar")
	UDataTable* StatusIconLookupTable;

private:
	UAbilitySystemComponent* _abilitySystemComponent;
	UBaseAttributeSet* _attributeSet;

	FRichImageRow* FindImageRow(FName TagOrId, bool bWarnIfMissing);

protected:
	virtual void OnActiveGameplayEffectAddedCallback(UAbilitySystemComponent* Target, const FGameplayEffectSpec& SpecApplied, FActiveGameplayEffectHandle ActiveHandle);
	virtual void OnRemoveGameplayEffectCallback(const FActiveGameplayEffect& EffectRemoved);
	virtual void HealthChanged(const FOnAttributeChangeData& Data);
};

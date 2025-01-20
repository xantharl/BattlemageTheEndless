// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "GameplayEffectTypes.h"
#include "../Abilities/BaseAttributeSet.h"
#include "Components/RichTextBlockImageDecorator.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Blueprint/WidgetTree.h"
#include "HealthBarWidget.generated.h"

USTRUCT(BlueprintType)
struct FStatusGridItem
{
	GENERATED_BODY()

public:
	FStatusGridItem() {}

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Health Bar")
	FGameplayTag StatusEffectTag;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Health Bar")
	int32 ActiveStacks;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Health Bar")
	FRichImageRow ImageRow;

	float TimeRemainingTotal;
	float TimeRemainingStack;
	float TimeInitialStack;

	bool operator==(const FStatusGridItem& Other) const
	{
		return StatusEffectTag == Other.StatusEffectTag;
	}

	FStatusGridItem(FGameplayTag statusEffectTag)
	{
		StatusEffectTag = statusEffectTag;
	}
};

/**
 * 
 */
UCLASS(Abstract, editinlinenew, BlueprintType, Blueprintable, meta = (DontUseGenericSpawnObject = "True", DisableNativeTick))
class BATTLEMAGETHEENDLESS_API UHealthBarWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void Init(UAbilitySystemComponent* abilitySystemComponent, UBaseAttributeSet* attributeSet);

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Health Bar")
	UDataTable* StatusIconLookupTable;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Health Bar")
	TArray<FStatusGridItem> StatusGrid;

	const FName CurrentHealthTextBlockName = FName("Current_Health");
	const FName ProgressBarName = FName("Health_Bar");

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Health Bar")
	int32 MAX_STATUS_EFFECTS = 6;

	void Reset();

private:
	UAbilitySystemComponent* _abilitySystemComponent;
	UBaseAttributeSet* _attributeSet;

	FRichImageRow* FindImageRow(FName TagOrId, bool bWarnIfMissing);

	void MoveStatusElementsAfterIndexDown(int32 startIndex);

	const wchar_t IconNameFormat[16] = TEXT("Status_%0d_Icon");
	const wchar_t ProgressNameFormat[20] = TEXT("Status_%0d_Progress");
	const wchar_t StackCountFormat[18] = TEXT("Status_%0d_Stacks");

protected:
	UFUNCTION()
	virtual void OnActiveGameplayEffectAddedCallback(UAbilitySystemComponent* Target, const FGameplayEffectSpec& SpecApplied, FActiveGameplayEffectHandle ActiveHandle);
	UFUNCTION()
	virtual void OnGameplayEffectStackChangeCallback(FActiveGameplayEffectHandle InHandle, int32 NewCount, int32 OldCount);
	UFUNCTION()
	virtual void OnRemoveGameplayEffectCallback(const FActiveGameplayEffect& EffectRemoved);
	virtual void HealthChanged(const FOnAttributeChangeData& Data);

	virtual void SetInitialDisplayValues();
	virtual void SubscribeToEffectChanges();

	FGameplayTagContainer OwnedStatusTags(const FGameplayEffectSpec& specApplied);

	FGameplayTagContainer OwnedStatusTags(const UGameplayEffect& effect);
};

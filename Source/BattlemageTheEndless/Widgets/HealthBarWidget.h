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
#include "Components/CanvasPanelSlot.h"
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
	float TimeInitialStack;
	float TimeElapsedStack = 0.f;

	bool operator==(const FStatusGridItem& Other) const
	{
		return StatusEffectTag == Other.StatusEffectTag;
	}

	FStatusGridItem(FGameplayTag statusEffectTag)
	{
		StatusEffectTag = statusEffectTag;
	}

	FVector2D InitialProgressSize;
	FTimerHandle StackTimer;
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

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Health Bar")
	int TickRate = 60;

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

	void ConfigureStatusAtIndex(int index, const FSlateBrush& statusIconBrush, FText stackCount);
	void SetVisibilityOfStatusAtIndex(int index, ESlateVisibility visibility);
	void MoveStatusElementsAfterIndexDown(int32 startIndex);
	void ClearTimerAtIndexIfActive(int statusGridItemIdx);

	FString IconNameFormat = TEXT("Status_%d_Icon");
	FString ProgressNameFormat = TEXT("Status_%d_Progress");
	FString StackCountFormat = TEXT("Status_%d_Stacks");

	static float GetTimeRemainingForStatusEffect(FStatusGridItem statusGridItem);

protected:
	UFUNCTION()
	virtual void OnActiveGameplayEffectAddedCallback(UAbilitySystemComponent* Target, const FGameplayEffectSpec& SpecApplied, FActiveGameplayEffectHandle ActiveHandle);

	UFUNCTION()
	void AdjustProgressIndicatorSize(int statusGridItemIdx);

	UFUNCTION()
	virtual void OnGameplayEffectStackChangeCallback(FActiveGameplayEffectHandle InHandle, int32 NewCount, int32 OldCount);
	UFUNCTION()
	virtual void OnRemoveGameplayEffectCallback(const FActiveGameplayEffect& EffectRemoved);
	virtual void HealthChanged(const FOnAttributeChangeData& Data);

	virtual void SetInitialDisplayValues();
	virtual void SubscribeToEffectChanges();

	UFUNCTION()
	void StartAdjustProgressIndicatorSizeTimer(int statusGridItemIdx);

	FGameplayTagContainer OwnedStatusTags(const FGameplayEffectSpec& specApplied);

	FGameplayTagContainer OwnedStatusTags(const UGameplayEffect& effect);
};

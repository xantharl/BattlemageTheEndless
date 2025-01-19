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

FRichImageRow* UHealthBarWidget::FindImageRow(FName TagOrId, bool bWarnIfMissing)
{
	// Get the row from the table
	if (!StatusIconLookupTable)
	{
		UE_LOG(LogTemp, Warning, TEXT("URichTextBlockImageDecorator::FindImageRow: No ImageSet specified!"));
		return nullptr;
	}

	FRichImageRow* row = StatusIconLookupTable->FindRow<FRichImageRow>(TagOrId, FString());
	if (bWarnIfMissing && !row)
	{
		UE_LOG(LogTemp, Warning, TEXT("URichTextBlockImageDecorator::FindImageRow: Unable to find row with TagOrId %s in ImageSet"), *TagOrId.ToString());
	}

	return row;
}

void UHealthBarWidget::MoveStatusElementsAfterIndexDown(int32 startIndex)
{
	auto tree = WidgetTree.Get();
	// hide the last active status icon (icons are 1 indexed)
	auto statusIconWidget = tree->FindWidget<UImage>(FName(FString::Printf(IconNameFormat, StatusGrid.Num())));
	auto progressIconWidget = tree->FindWidget<UImage>(FName(FString::Printf(ProgressNameFormat, StatusGrid.Num())));
	auto stackCounterWidget = tree->FindWidget<UTextBlock>(FName(FString::Printf(StackCountFormat, StatusGrid.Num())));

	statusIconWidget->SetVisibility(ESlateVisibility::Hidden);
	progressIconWidget->SetVisibility(ESlateVisibility::Hidden);
	stackCounterWidget->SetVisibility(ESlateVisibility::Hidden);

	// move all elements after the start index down one
	for (int i = startIndex; i < StatusGrid.Num() - 1; i++)
	{
		statusIconWidget = tree->FindWidget<UImage>(FName(FString::Printf(IconNameFormat, i)));
		progressIconWidget = tree->FindWidget<UImage>(FName(FString::Printf(ProgressNameFormat, i)));
		stackCounterWidget = tree->FindWidget<UTextBlock>(FName(FString::Printf(StackCountFormat, i)));

		auto nextStatusIconWidget = tree->FindWidget<UImage>(FName(FString::Printf(IconNameFormat, i + 1)));
		auto nextProgressIconWidget = tree->FindWidget<UImage>(FName(FString::Printf(ProgressNameFormat, i + 1)));
		auto nextStackCounterWidget = tree->FindWidget<UTextBlock>(FName(FString::Printf(StackCountFormat, i + 1)));

		statusIconWidget->SetBrush(nextStatusIconWidget->GetBrush());
		progressIconWidget->SetBrush(nextProgressIconWidget->GetBrush());
		stackCounterWidget->SetText(nextStackCounterWidget->GetText());
	}
}

void UHealthBarWidget::OnActiveGameplayEffectAddedCallback(UAbilitySystemComponent* Target, const FGameplayEffectSpec& SpecApplied, FActiveGameplayEffectHandle ActiveHandle)
{
	auto appliedStatusTags = OwnedStatusTags(SpecApplied);
	// This applied spec has no status effect tags, so we don't care about it
	if (appliedStatusTags.Num() == 0)
	{
		return;
	}
	
	// for each appied status tag, increment stack count of or add the status grid item
	for (auto appliedTag : appliedStatusTags)
	{
		auto statusGridItemIdx = StatusGrid.Find(FStatusGridItem(appliedTag));
		if (statusGridItemIdx != INDEX_NONE)
		{
			StatusGrid[statusGridItemIdx].ActiveStacks++;

			// find the counter widget and update the stacks
			auto stackCounterWidget = WidgetTree.Get()->FindWidget<UTextBlock>(FName(FString::Printf(StackCountFormat, statusGridItemIdx+1)));
			if (stackCounterWidget)
				stackCounterWidget->SetText(FText::FromString(FString::FromInt(StatusGrid[statusGridItemIdx].ActiveStacks)));

			continue;
		}

		statusGridItemIdx = 0;
		auto imageRow = FindImageRow(appliedTag.GetTagName(), true);

		FStatusGridItem newItem;
		newItem.StatusEffectTag = appliedTag;
		newItem.ActiveStacks = 1;
		if (imageRow != nullptr)
			newItem.ImageRow = *imageRow;
		StatusGrid.Add(newItem);

		// enable the status icon widget, its counter, and its progress bar
		auto statusIconWidget = WidgetTree.Get()->FindWidget<UImage>(FName(FString::Printf(IconNameFormat, statusGridItemIdx+1)));

		// TODO: Implement progress bar
		//auto progressBarWidget = WidgetTree.Get()->FindWidget<UImage>(FName(FString::Printf(ProgressNameFormat)));
		if (statusIconWidget)
		{
			statusIconWidget->SetBrush(newItem.ImageRow.Brush);
			statusIconWidget->SetVisibility(ESlateVisibility::Visible);
		}
		else
			return;

		/*if (progressBarWidget)
		{
			auto maxSize = statusIconWidget->GetDesiredSize();
			progressBarWidget->SetDesiredSizeOverride(FVector2D(maxSize.X, maxSize.Y));
			progressBarWidget->SetVisibility(ESlateVisibility::Visible);
		}*/

		auto stackCounterWidget = WidgetTree.Get()->FindWidget<UTextBlock>(FName(FString::Printf(StackCountFormat, statusGridItemIdx + 1)));
		if (stackCounterWidget)
		{
			stackCounterWidget->SetText(FText::FromString(FString::FromInt(StatusGrid[statusGridItemIdx].ActiveStacks)));
			stackCounterWidget->SetVisibility(ESlateVisibility::Visible);
		}

		// subscribe to stack changes
		_abilitySystemComponent->OnGameplayEffectStackChangeDelegate(ActiveHandle)->AddUObject(this, &UHealthBarWidget::OnGameplayEffectStackChangeCallback);
	}
}

void UHealthBarWidget::OnGameplayEffectStackChangeCallback(FActiveGameplayEffectHandle InHandle, int32 NewCount, int32 OldCount)
{
	auto appliedStatusTags = OwnedStatusTags(_abilitySystemComponent->GetActiveGameplayEffect(InHandle)->Spec);
	// This applied spec has no status effect tags, so we don't care about it
	if (appliedStatusTags.Num() == 0)
	{
		return;
	}

	// for each appied status tag, update stack count and reset progress bar
	for (auto appliedTag : appliedStatusTags)
	{
		auto statusGridItemIdx = StatusGrid.Find(FStatusGridItem(appliedTag));
		if (statusGridItemIdx == INDEX_NONE)
		{
			UE_LOG(LogTemp, Warning, TEXT("UHealthBarWidget::OnGameplayEffectStackChangeCallback: Could not find status grid item for tag %s"), *appliedTag.GetTagName().ToString());
			return;
		}

		StatusGrid[statusGridItemIdx].ActiveStacks = NewCount;

		// find the counter widget and update the stacks
		auto stackCounterWidget = WidgetTree.Get()->FindWidget<UTextBlock>(FName(FString::Printf(StackCountFormat, statusGridItemIdx + 1)));
		if (stackCounterWidget)
			stackCounterWidget->SetText(FText::FromString(FString::FromInt(NewCount)));
	}
}

void UHealthBarWidget::OnRemoveGameplayEffectCallback(const FActiveGameplayEffect& EffectRemoved)
{
	auto removedStatusTags = OwnedStatusTags(EffectRemoved.Spec);
	// This applied spec has no status effect tags, so we don't care about it
	if (removedStatusTags.Num() == 0)
	{
		return;
	}

	// for each appied status tag, increment stack count of or add the status grid item
	for (auto removedTag : removedStatusTags)
	{
		auto statusGridItemIdx = StatusGrid.Find(FStatusGridItem(removedTag));
		if (statusGridItemIdx == INDEX_NONE)
		{
			UE_LOG(LogTemp, Warning, TEXT("UHealthBarWidget::OnRemoveGameplayEffectCallback: Could not find status grid item for tag %s"), *removedTag.GetTagName().ToString());
			return;
		}

		// Hide all visual components
		// enable the status icon widget, its counter, and its progress bar
		auto statusIconWidget = WidgetTree.Get()->FindWidget<UImage>(FName(FString::Printf(IconNameFormat, statusGridItemIdx + 1)));

		// TODO: Implement progress bar
		//auto progressBarWidget = WidgetTree.Get()->FindWidget<UImage>(FName(FString::Printf(ProgressNameFormat)));
		if (statusIconWidget)
		{
			statusIconWidget->SetVisibility(ESlateVisibility::Hidden);
		}
		else
			return;

		/*if (progressBarWidget)
		{
			auto maxSize = statusIconWidget->GetDesiredSize();
			progressBarWidget->SetDesiredSizeOverride(FVector2D(maxSize.X, maxSize.Y));
			progressBarWidget->SetVisibility(ESlateVisibility::Visible);
		}*/

		auto stackCounterWidget = WidgetTree.Get()->FindWidget<UTextBlock>(FName(FString::Printf(StackCountFormat, statusGridItemIdx + 1)));
		if (stackCounterWidget)
		{
			stackCounterWidget->SetVisibility(ESlateVisibility::Hidden);
		}

		// if this was not the last element in the StatusGrid, move the rest up a spot
		if (statusGridItemIdx != StatusGrid.Num() - 1)
		{ 
			MoveStatusElementsAfterIndexDown(statusGridItemIdx);
		}

		StatusGrid.RemoveAt(statusGridItemIdx);
	}
}

void UHealthBarWidget::HealthChanged(const FOnAttributeChangeData& Data)
{
	// Update health bar
	auto tree = WidgetTree.Get();
	auto currentHealthWidget = tree->FindWidget<UTextBlock>(FName("Current_Health"));
	if (currentHealthWidget)
	{
		currentHealthWidget->SetText(FText::FromString(FString::FromInt(Data.NewValue)));
	}

	auto healthBarWidget = tree->FindWidget<UProgressBar>(FName("Health_Bar"));
	if (healthBarWidget)
	{
		healthBarWidget->SetPercent(Data.NewValue / _attributeSet->MaxHealth.GetBaseValue());
	}
}

FGameplayTagContainer UHealthBarWidget::OwnedStatusTags(const FGameplayEffectSpec& specApplied)
{
	return OwnedStatusTags(*specApplied.Def);
}

FGameplayTagContainer UHealthBarWidget::OwnedStatusTags(const UGameplayEffect& effect)
{
	//return SpecApplied.Def->GetAssetTags().Filter(FGameplayTagContainer(FGameplayTag::RequestGameplayTag("Status")));
	auto baseTagContainer = FGameplayTagContainer(FGameplayTag::RequestGameplayTag("GameplayCue.Status"));

	FGameplayTagContainer statusTags;
	for (auto gameplayCue : effect.GameplayCues)
	{
		statusTags.AppendTags(gameplayCue.GameplayCueTags.Filter(baseTagContainer));
	}
	return statusTags;
}
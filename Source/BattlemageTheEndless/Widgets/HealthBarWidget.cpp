// Fill out your copyright notice in the Description page of Project Settings.


#include "HealthBarWidget.h"

void UHealthBarWidget::Init(UAbilitySystemComponent* abilitySystemComponent, UBaseAttributeSet* attributeSet)
{
	_abilitySystemComponent = abilitySystemComponent;
	_attributeSet = attributeSet;
	SetInitialDisplayValues();
	SubscribeToEffectChanges();
}

void UHealthBarWidget::SetInitialDisplayValues()
{
	auto tree = WidgetTree.Get();
	auto currentHealthWidget = tree->FindWidget<UTextBlock>(CurrentHealthTextBlockName);
	if (currentHealthWidget && _attributeSet)
	{
		currentHealthWidget->SetText(FText::FromString(FString::FromInt(_attributeSet->Health.GetBaseValue())));
	}

	auto healthBarWidget = tree->FindWidget<UProgressBar>(ProgressBarName);
	if (healthBarWidget)
	{
		healthBarWidget->SetPercent(1.0f);
	}
}

void UHealthBarWidget::SubscribeToEffectChanges()
{
	_abilitySystemComponent->OnActiveGameplayEffectAddedDelegateToSelf.AddUObject(this, &UHealthBarWidget::OnActiveGameplayEffectAddedCallback);
	_abilitySystemComponent->OnAnyGameplayEffectRemovedDelegate().AddUObject(this, &UHealthBarWidget::OnRemoveGameplayEffectCallback);
	_abilitySystemComponent->GetGameplayAttributeValueChangeDelegate(_attributeSet->GetHealthAttribute()).AddUObject(this, &UHealthBarWidget::HealthChanged);
}

void UHealthBarWidget::Reset()
{
	SetInitialDisplayValues();
	StatusGrid.Empty();
	auto tree = WidgetTree.Get();
	for (int i = 0; i < MAX_STATUS_EFFECTS; ++i)
	{
		auto statusIconWidget = tree->FindWidget<UImage>(FName(IconNameFormat.Replace(TEXT("%d"), *FString::FromInt(i))));
		auto progressIconWidget = tree->FindWidget<UImage>(FName(ProgressNameFormat.Replace(TEXT("%d"), *FString::FromInt(i))));
		auto stackCounterWidget = tree->FindWidget<UTextBlock>(FName(StackCountFormat.Replace(TEXT("%d"), *FString::FromInt(i))));

		statusIconWidget->SetVisibility(ESlateVisibility::Hidden);
		progressIconWidget->SetVisibility(ESlateVisibility::Hidden);
		stackCounterWidget->SetVisibility(ESlateVisibility::Hidden);
	
	}
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

void UHealthBarWidget::ConfigureStatusAtIndex(int index, const FSlateBrush& statusIconBrush, FText stackCount)
{
	auto tree = WidgetTree.Get();
	auto statusIconWidget = tree->FindWidget<UImage>(FName(IconNameFormat.Replace(TEXT("%d"), *FString::FromInt(index))));
	auto stackCounterWidget = tree->FindWidget<UTextBlock>(FName(StackCountFormat.Replace(TEXT("%d"), *FString::FromInt(index))));

	statusIconWidget->SetBrush(statusIconBrush);
	stackCounterWidget->SetText(stackCount);
}

void UHealthBarWidget::SetVisibilityOfStatusAtIndex(int index, ESlateVisibility visibility)
{
	auto tree = WidgetTree.Get();

	// hide the last active status icon (icons are 1 indexed)
	auto statusIconWidget = tree->FindWidget<UImage>(FName(IconNameFormat.Replace(TEXT("%d"), *FString::FromInt(index))));
	auto progressIconWidget = tree->FindWidget<UImage>(FName(ProgressNameFormat.Replace(TEXT("%d"), *FString::FromInt(index))));
	auto stackCounterWidget = tree->FindWidget<UTextBlock>(FName(StackCountFormat.Replace(TEXT("%d"), *FString::FromInt(index))));

	statusIconWidget->SetVisibility(visibility);
	progressIconWidget->SetVisibility(visibility);
	stackCounterWidget->SetVisibility(visibility);

	// reset the size on hide to make sure we aren't shrinking it each time
	if (visibility == ESlateVisibility::Hidden)
	{
		auto canvasPanelSlot = Cast<UCanvasPanelSlot>(progressIconWidget->Slot);
		canvasPanelSlot->SetSize(StatusGrid[index].InitialProgressSize);
	}
}

void UHealthBarWidget::MoveStatusElementsAfterIndexDown(int32 startIndex)
{
	auto tree = WidgetTree.Get();

	// move all elements after the start index down one
	for (int i = startIndex; i < StatusGrid.Num()-1; i++)
	{
	auto nextStatusIconWidget = tree->FindWidget<UImage>(FName(IconNameFormat.Replace(TEXT("%d"), *FString::FromInt(i+1))));
	auto nextStackCounterWidget = tree->FindWidget<UTextBlock>(FName(StackCountFormat.Replace(TEXT("%d"), *FString::FromInt(i+1))));

		ConfigureStatusAtIndex(i, nextStatusIconWidget->GetBrush(), nextStackCounterWidget->GetText());
	}

	// hide the last active status icon
	SetVisibilityOfStatusAtIndex(StatusGrid.Num()-1, ESlateVisibility::Hidden);
}

float UHealthBarWidget::GetTimeRemainingForStatusEffect(FStatusGridItem statusGridItem)
{
	return (statusGridItem.ActiveStacks * statusGridItem.TimeInitialStack) - statusGridItem.TimeElapsedStack;
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
			auto stackCounterWidget = WidgetTree.Get()->FindWidget<UTextBlock>(FName(StackCountFormat.Replace(TEXT("%d"), *FString::FromInt(statusGridItemIdx))));
			if (stackCounterWidget)
				stackCounterWidget->SetText(FText::FromString(FString::FromInt(StatusGrid[statusGridItemIdx].ActiveStacks)));

			continue;
		}

		statusGridItemIdx = StatusGrid.Num();
		auto imageRow = FindImageRow(appliedTag.GetTagName(), true);

		FStatusGridItem newItem;
		newItem.StatusEffectTag = appliedTag;
		newItem.ActiveStacks = 1;
		if (imageRow != nullptr)
			newItem.ImageRow = *imageRow;
		// This is set properly the first time adjust size is called
		newItem.InitialProgressSize = FVector2D::ZeroVector;
		newItem.TimeInitialStack = SpecApplied.GetDuration();
		StatusGrid.Add(newItem);

		ConfigureStatusAtIndex(statusGridItemIdx, newItem.ImageRow.Brush, FText::FromString(FString::FromInt(StatusGrid[statusGridItemIdx].ActiveStacks)));
		SetVisibilityOfStatusAtIndex(statusGridItemIdx, ESlateVisibility::Visible);

		// subscribe to stack changes
		_abilitySystemComponent->OnGameplayEffectStackChangeDelegate(ActiveHandle)->AddUObject(this, &UHealthBarWidget::OnGameplayEffectStackChangeCallback);
		StartAdjustProgressIndicatorSizeTimer(statusGridItemIdx);
	}

	// If we have more than one active effect, make sure they're displayed in order of total duration descending
	if (StatusGrid.Num() > 1)
	{
		// Copy status grid
		auto sortedStatusGrid = StatusGrid;
		sortedStatusGrid.Sort([](const FStatusGridItem& A, const FStatusGridItem& B) {
			return GetTimeRemainingForStatusEffect(A) > GetTimeRemainingForStatusEffect(B);
		});

		bool orderChanged = false;
		// Otherwise update the display order
		for (int i = 0; i < sortedStatusGrid.Num(); i++)
		{
			// If the appropriate effect is already at this index, continue
			if (sortedStatusGrid[i] == StatusGrid[i])
				continue;

			if (!orderChanged)
				orderChanged = true;

			// Otherwise update accordingly
			ConfigureStatusAtIndex(i, sortedStatusGrid[i].ImageRow.Brush, FText::FromString(FString::FromInt(sortedStatusGrid[i].ActiveStacks)));
		}

		if (orderChanged)
			StatusGrid = sortedStatusGrid;
	}
}

void UHealthBarWidget::AdjustProgressIndicatorSize(int statusGridItemIdx)
{
	if (StatusGrid.Num() <= statusGridItemIdx)
		return;

	auto tree = WidgetTree.Get();
	auto progressIconWidget = tree->FindWidget<UImage>(FName(ProgressNameFormat.Replace(TEXT("%d"), *FString::FromInt(statusGridItemIdx))));
	auto canvasPanelSlot = Cast<UCanvasPanelSlot>(progressIconWidget->Slot);
	// If we cannot fetch the slot as a canvas panel slot, we cannot adjust the size
	if (!canvasPanelSlot)
		return;

	// Init Initial Progress Size if needed
	if (StatusGrid[statusGridItemIdx].InitialProgressSize == FVector2D::ZeroVector)
		StatusGrid[statusGridItemIdx].InitialProgressSize = canvasPanelSlot->GetSize();

	auto timer = StatusGrid[statusGridItemIdx].StackTimer;
	if (!GetWorld()->GetTimerManager().IsTimerActive(timer))
		return;

	auto elapsed = GetWorld()->GetTimerManager().GetTimerElapsed(timer);
	StatusGrid[statusGridItemIdx].TimeElapsedStack += elapsed;
	auto newSize = FVector2D(StatusGrid[statusGridItemIdx].InitialProgressSize.X, 
		StatusGrid[statusGridItemIdx].InitialProgressSize.Y * (1.f - (StatusGrid[statusGridItemIdx].TimeElapsedStack / StatusGrid[statusGridItemIdx].TimeInitialStack)));

	//newSize.Y = FMath::Clamp(newSize.Y, 0.f, StatusGrid[statusGridItemIdx].InitialProgressSize.Y);
	canvasPanelSlot->SetSize(newSize);
	
}

void UHealthBarWidget::StartAdjustProgressIndicatorSizeTimer(int statusGridItemIdx)
{
	FTimerDelegate timerDelegate = FTimerDelegate::CreateUObject(this, &UHealthBarWidget::AdjustProgressIndicatorSize, statusGridItemIdx);
	StatusGrid[statusGridItemIdx].TimeElapsedStack = 0.f;
	GetWorld()->GetTimerManager().SetTimer(StatusGrid[statusGridItemIdx].StackTimer, timerDelegate, 1.f/(float)TickRate, true);
}

void UHealthBarWidget::ClearTimerAtIndexIfActive(int statusGridItemIdx)
{
	if (StatusGrid.Num() <= statusGridItemIdx)
		return;

	auto timer = StatusGrid[statusGridItemIdx].StackTimer;
	if (timer.IsValid())
	{
		GetWorld()->GetTimerManager().ClearTimer(timer);
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
		auto stackCounterWidget = WidgetTree.Get()->FindWidget<UTextBlock>(FName(StackCountFormat.Replace(TEXT("%d"), *FString::FromInt(statusGridItemIdx))));
		if (stackCounterWidget)
			stackCounterWidget->SetText(FText::FromString(FString::FromInt(NewCount)));

		StartAdjustProgressIndicatorSizeTimer(statusGridItemIdx);
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

		ClearTimerAtIndexIfActive(statusGridItemIdx);

		// if this effect is the last one, remove the status grid item and hide the visual components
		if (statusGridItemIdx == StatusGrid.Num() - 1)
		{
			// Hide all visual components
			SetVisibilityOfStatusAtIndex(statusGridItemIdx, ESlateVisibility::Hidden);
		}
		else
		{
			// if this was not the last element in the StatusGrid, move the rest up a spot
			MoveStatusElementsAfterIndexDown(statusGridItemIdx);
		}
		StatusGrid.RemoveAt(statusGridItemIdx);
	}
}

void UHealthBarWidget::HealthChanged(const FOnAttributeChangeData& Data)
{
	// Update health bar
	auto tree = WidgetTree.Get();
	auto currentHealthWidget = tree->FindWidget<UTextBlock>(CurrentHealthTextBlockName);
	if (currentHealthWidget)
	{
		currentHealthWidget->SetText(FText::FromString(FString::FromInt(Data.NewValue)));
	}

	auto healthBarWidget = tree->FindWidget<UProgressBar>(ProgressBarName);
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
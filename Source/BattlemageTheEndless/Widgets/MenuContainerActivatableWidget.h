// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "Widgets/CommonActivatableWidgetContainer.h"
#include "MainMenuActivatableWidget.h"
#include "MenuContainerActivatableWidget.generated.h"

/**
 * 
 */
UCLASS()
class BATTLEMAGETHEENDLESS_API UMenuContainerActivatableWidget : public UCommonActivatableWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Menu", meta = (BindWidget))
	class UCommonActivatableWidgetStack* MainMenuStack = nullptr;	

	virtual void NativeOnActivated();

private:
	UMainMenuActivatableWidget* MainMenuWidget = nullptr;
};

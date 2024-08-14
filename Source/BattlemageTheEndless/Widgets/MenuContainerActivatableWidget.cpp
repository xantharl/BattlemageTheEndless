// Fill out your copyright notice in the Description page of Project Settings.


#include "MenuContainerActivatableWidget.h"

void UMenuContainerActivatableWidget::NativeOnActivated()
{
	UCommonActivatableWidget::NativeOnActivated();
	//if (MainMenuStack && !MainMenuWidget && MainMenuStack->GetRootContent())
	//{
	//	// set the main menu widget to the root content
	//	MainMenuWidget = Cast<UMainMenuActivatableWidget>(MainMenuStack->GetRootContent());
	//}
	//else if (MainMenuWidget && MainMenuStack && !MainMenuStack->GetWidgetList().Contains(MainMenuWidget))
	//{
	//	MainMenuStack->AddWidgetInstance(*MainMenuWidget);
	//}
	//else if (MainMenuWidget && MainMenuStack && MainMenuStack->GetWidgetList().Contains(MainMenuWidget))
	//{
	//	MainMenuWidget->ActivateWidget();
	//}
}

void UMenuContainerActivatableWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// Bind delegates here.
}
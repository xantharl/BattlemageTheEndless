// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
//#include "Blueprint/UserWidget.h"
#include "BattlemageHud.generated.h"

/**
 * 
 */
UCLASS()
class BATTLEMAGETHEENDLESS_API ABattlemageHud : public AHUD
{
	GENERATED_BODY()
public:
	ABattlemageHud();

	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = HUD, meta = (AllowPrivateAccess = "true"))
	//UUserWidget* CrosshairWidget;

	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = HUD, meta = (AllowPrivateAccess = "true"))
	//UUserWidget* HealthWidget;

	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = HUD, meta = (AllowPrivateAccess = "true"))
	//UUserWidget* ManaWidget;

protected:
	virtual void BeginPlay();
};

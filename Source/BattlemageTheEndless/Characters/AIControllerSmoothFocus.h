// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Kismet/KismetMathLibrary.h"
#include "AIControllerSmoothFocus.generated.h"

/**
 * 
 */
UCLASS()
class BATTLEMAGETHEENDLESS_API AAIControllerSmoothFocus : public AAIController
{
	GENERATED_BODY()
protected:
	FRotator SmoothTargetRotation;
	 
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float SmoothFocusInterpSpeed = 30.0f;
	 
public:
	virtual void UpdateControlRotation(float DeltaTime, bool bUpdatePawn) override;

	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;
};

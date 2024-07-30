// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/CapsuleComponent.h"
#include "CheckPoint.generated.h"

UCLASS()
class BATTLEMAGETHEENDLESS_API ACheckPoint : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ACheckPoint();

	// create a capsule component
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CheckPoint")
	UCapsuleComponent* CapsuleComponent;
};
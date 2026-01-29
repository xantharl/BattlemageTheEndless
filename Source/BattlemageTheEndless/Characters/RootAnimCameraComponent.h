// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BattlemageTheEndlessCharacter.h"
#include "Components/ActorComponent.h"
#include "RootAnimCameraComponent.generated.h"

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class BATTLEMAGETHEENDLESS_API URootAnimCameraComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	URootAnimCameraComponent();
	
	virtual void Activate(bool bReset = false) override;
	virtual void Deactivate() override;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;
	
	UPROPERTY(BlueprintReadOnly, Category = "Character")
	ACharacter* Owner;
	
	UPROPERTY(BlueprintReadOnly, Category = "Character")
	UCharacterMovementComponent* Movement;
	
	UPROPERTY(BlueprintReadOnly, Category = "Character")
	USceneComponent* CameraComponent;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
		
};

// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BattlemageTheEndlessCharacter.h"
#include "GameFramework/Character.h"
#include "TrainingDummyActor.generated.h"

UCLASS()
class BATTLEMAGETHEENDLESS_API ATrainingDummyActor : public ABattlemageTheEndlessCharacter
{
	GENERATED_BODY()

public:
	ATrainingDummyActor(const FObjectInitializer& ObjectInitializer);

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	time_t LastHitTime;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health")
	float WaitBeforeResetHealthSeconds = 3.0f;

	FTimerHandle ResetHealthTimer;

	UFUNCTION()
	void ResetHealth();

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	virtual void ApplyDamage(float damage);
};

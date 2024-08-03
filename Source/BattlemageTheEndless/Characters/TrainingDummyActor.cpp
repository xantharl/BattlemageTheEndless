// Fill out your copyright notice in the Description page of Project Settings.


#include "TrainingDummyActor.h"

// Sets default values
ATrainingDummyActor::ATrainingDummyActor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UBMageCharacterMovementComponent>(ACharacter::CharacterMovementComponentName))
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void ATrainingDummyActor::BeginPlay()
{
	Super::BeginPlay();
	
}

void ATrainingDummyActor::ResetHealth()
{
	// Return if this wasn't invoked by the latest hit
	if (time(0) - LastHitTime > 3.0f)
	{
		return;
	}

	Health = MaxHealth;
	GetWorldTimerManager().ClearTimer(ResetHealthTimer);
}

// Called every frame
void ATrainingDummyActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void ATrainingDummyActor::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

void ATrainingDummyActor::ApplyDamage(float damage)
{
	LastHitTime = time(0);
	GetWorld()->GetTimerManager().SetTimer(ResetHealthTimer, this, &ATrainingDummyActor::ResetHealth, WaitBeforeResetHealthSeconds, false);

	ABattlemageTheEndlessCharacter::ApplyDamage(damage);
}

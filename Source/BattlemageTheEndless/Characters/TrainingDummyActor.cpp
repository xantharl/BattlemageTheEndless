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

	HealthChangedDelegateHandle = AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(AttributeSet->GetHealthAttribute())
		.AddUObject(this, &ATrainingDummyActor::HealthChanged);
}

void ATrainingDummyActor::ResetHealth()
{
	AttributeSet->Health.SetCurrentValue(AttributeSet->MaxHealth.GetBaseValue());
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

void ATrainingDummyActor::HealthChanged(const FOnAttributeChangeData& Data)
{
	Super::HealthChanged(Data);

	if (Data.NewValue <= AttributeSet->GetHealth())
	{
		// Replaces existing timer if present
		GetWorld()->GetTimerManager().SetTimer(ResetHealthTimer, this, &ATrainingDummyActor::ResetHealth, WaitBeforeResetHealthSeconds, false);
	}
}

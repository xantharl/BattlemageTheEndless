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
		.AddUObject(this, &ATrainingDummyActor::OnHealthChanged);
}

void ATrainingDummyActor::ResetHealth()
{
	// Passing an empty tag container is pretty hacky but let's see if matching no tags works to fetch all effects
	FGameplayEffectQuery allEffectsQuery = FGameplayEffectQuery::MakeQuery_MatchNoOwningTags(FGameplayTagContainer());
	AbilitySystemComponent->RemoveActiveEffects(allEffectsQuery);

	AttributeSet->Health.SetCurrentValue(AttributeSet->MaxHealth.GetBaseValue());
	AttributeSet->Health.SetBaseValue(AttributeSet->MaxHealth.GetBaseValue());
	GetWorldTimerManager().ClearTimer(ResetHealthTimer);

	if (UHealthBarWidget* HealthBarWidget = GetHealthBarWidget())
	{
		HealthBarWidget->Reset();
	}
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

void ATrainingDummyActor::OnHealthChanged(const FOnAttributeChangeData& Data)
{
	Super::OnHealthChanged(Data);

	if (Data.NewValue <= AttributeSet->GetHealth())
	{
		/*if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, FString::Printf(TEXT("Training Dummy health changed: %f -> %f"), Data.OldValue, Data.NewValue));
		}*/
		// Replaces existing timer if present
		GetWorld()->GetTimerManager().SetTimer(ResetHealthTimer, this, &ATrainingDummyActor::ResetHealth, WaitBeforeResetHealthSeconds, false);
	}
	// else
	// {
	// 	if(GEngine)
	// 	{
	// 		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("Training Dummy Health Restored"));
	// 	}
	// }
}

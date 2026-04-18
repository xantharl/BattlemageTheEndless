// Fill out your copyright notice in the Description page of Project Settings.

#include "GA_Launched.h"
#include "GameplayTagsModule.h"

UGA_Launched::UGA_Launched()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;

	FAbilityTriggerData TriggerData;
	TriggerData.TriggerTag = FGameplayTag::RequestGameplayTag(FName("Ability.React.Launched"));
	TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	AbilityTriggers.Add(TriggerData);
}

void UGA_Launched::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	ACharacter* TargetCharacter = Cast<ACharacter>(ActorInfo->AvatarActor.Get());
	if (!TargetCharacter)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	const AActor* SourceActor = TriggerEventData ? Cast<AActor>(TriggerEventData->Instigator.Get()) : nullptr;

	FVector TransformedKnockback = KnockBackVelocity;

	if (!SourceActor || SourceActor == TargetCharacter)
	{
		TransformedKnockback = TransformedKnockback.RotateAngleAxis(TargetCharacter->GetActorRotation().Yaw, FVector::ZAxisVector);
	}
	else
	{
		FVector Direction = TargetCharacter->GetActorLocation() - SourceActor->GetActorLocation();
		Direction.Normalize();

		if (!ConsiderZAxis)
		{
			Direction.Z = 0;
		}
		else if (auto BMageCharacter = Cast<ABattlemageTheEndlessCharacter>(SourceActor))
		{
			FVector CameraForward = BMageCharacter->GetActiveCamera()->GetComponentRotation().Vector();
			Direction.Z = CameraForward.Z;
		}

		TransformedKnockback = Direction.Rotation().RotateVector(KnockBackVelocity);

		auto TargetMovement = TargetCharacter->GetMovementComponent();
		if (TargetMovement && !TargetMovement->IsFalling() && TransformedKnockback.Z < 0)
		{
			TransformedKnockback.Z = 0;
		}
	}

	UBMageCharacterMovementComponent* MovementComponent = Cast<UBMageCharacterMovementComponent>(TargetCharacter->GetCharacterMovement());
	if (MovementComponent)
	{
		// While airborne, the attacker's root motion can drive their capsule into the launched character.
		// UCharacterMovementComponent resolves the overlap by applying a velocity impulse to the launched
		// character (MaxDepenetrationWithPawnAsCharacter), which looks like an extra knockback hit.
		// Zero it out for the duration of the ability so depenetration can't impart force mid-launch.
		_savedMaxDepenetration = MovementComponent->MaxDepenetrationWithPawn;
		MovementComponent->MaxDepenetrationWithPawn = 0.f;
	}

	_launchedCharacter = TargetCharacter;
	TargetCharacter->LandedDelegate.AddDynamic(this, &UGA_Launched::OnTargetLanded);

	TargetCharacter->LaunchCharacter(TransformedKnockback, true, true);

	if (GravityScaleOverTime && MovementComponent)
	{
		MovementComponent->BeginGravityOverTime(GravityScaleOverTime);
		MovementComponent->OnGravityOverTimeEnded.AddDynamic(this, &UGA_Launched::OnGravityCurveEnded);
	}

	if (CancelVelocityAfter > KINDA_SMALL_NUMBER)
	{
		TargetCharacter->GetWorld()->GetTimerManager().SetTimer(
			CancelVelocityTimerHandle,
			[this, TargetCharacter]() { CancelVelocity(TargetCharacter); },
			CancelVelocityAfter,
			false
		);
	}
}

void UGA_Launched::OnTargetLanded(const FHitResult& Hit)
{
	if (IsActive())
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_Launched::OnGravityCurveEnded()
{
	if (IsActive())
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_Launched::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (_launchedCharacter.IsValid())
		_launchedCharacter->LandedDelegate.RemoveDynamic(this, &UGA_Launched::OnTargetLanded);

	if (ACharacter* TargetCharacter = Cast<ACharacter>(ActorInfo->AvatarActor.Get()))
	{
		if (UBMageCharacterMovementComponent* MC = Cast<UBMageCharacterMovementComponent>(TargetCharacter->GetCharacterMovement()))
		{
			MC->MaxDepenetrationWithPawn = _savedMaxDepenetration;
			MC->OnGravityOverTimeEnded.RemoveDynamic(this, &UGA_Launched::OnGravityCurveEnded);
		}
	}

	if (UWorld* World = GetWorld())
		World->GetTimerManager().ClearTimer(CancelVelocityTimerHandle);

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGA_Launched::CancelVelocity(ACharacter* Character)
{
	if (!Character)
		return;

	if (auto Movement = Character->GetCharacterMovement())
	{
		Movement->StopMovementImmediately();
		Movement->SetMovementMode(MOVE_None);
	}
}

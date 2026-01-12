// Fill out your copyright notice in the Description page of Project Settings.


#include "BMageCharacterMovementComponent.h"

#include "BattlemageTheEndlessPlayerController.h"
#include "BattlemageTheEndless/Abilities/EventData/DodgeEventData.h"

UBMageCharacterMovementComponent::UBMageCharacterMovementComponent()
{
	// MovementAbilities.Add(MovementAbilityType::WallRun, CreateDefaultSubobject<UWallRunAbility>(TEXT("WallRun")));
	// MovementAbilities.Add(MovementAbilityType::Launch, CreateDefaultSubobject<ULaunchAbility>(TEXT("Launch")));
	// MovementAbilities.Add(MovementAbilityType::Sprint, CreateDefaultSubobject<USprintAbility>(TEXT("Sprint")));
	// MovementAbilities.Add(MovementAbilityType::Slide, CreateDefaultSubobject<USlideAbility>(TEXT("Slide")));
	// MovementAbilities.Add(MovementAbilityType::Vault, CreateDefaultSubobject<UVaultAbility>(TEXT("Vault")));
	// MovementAbilities.Add(MovementAbilityType::Dodge, CreateDefaultSubobject<UDodgeAbility>(TEXT("Dodge")));

	AirControl = BaseAirControl;
	GetNavAgentPropertiesRef().bCanCrouch = true;
}

void UBMageCharacterMovementComponent::InitAbilities(ACharacter* Character, USkeletalMeshComponent* Mesh)
{
	for (auto& ability : MovementAbilities)
	{
		ability.Value->Init(this, Character, Mesh);
		ability.Value->OnMovementAbilityBegin.AddDynamic(this, &UBMageCharacterMovementComponent::OnMovementAbilityBegin);
		ability.Value->OnMovementAbilityEnd.AddDynamic(this, &UBMageCharacterMovementComponent::OnMovementAbilityEnd);
		ability.Value->OnMovementAbilityShouldTransitionOut.AddDynamic(this, &UBMageCharacterMovementComponent::OnMovementAbilityShouldTransitionOut);
	}
}

void UBMageCharacterMovementComponent::ApplyInput()
{
	FVector forwardVector = Velocity;
	forwardVector = FVector(forwardVector.RotateAngleAxis(GetLastUpdateRotation().GetInverse().Yaw, FVector::ZAxisVector));

	// if we've started moving backwards, set the speed to ReverseSpeed
	if (forwardVector.X < 0 && MaxWalkSpeed != ReverseSpeed)
	{
		MaxWalkSpeed = ReverseSpeed;
	}
	// otherwise set it to WalkSpeed
	else if (forwardVector.X >= 0 && MaxWalkSpeed == ReverseSpeed)
	{
		ResetWalkSpeed();
	}
}

void UBMageCharacterMovementComponent::BeginGravityOverTime(UCurveFloat* gravityCurve)
{
	_initialGravityScale = GravityScale;
	_activeGravityCurve = gravityCurve;
	float _ = 0.f;
	float maxTime = 0.f;
	gravityCurve->GetTimeRange(_, maxTime);
	_gravityCurveDuration = milliseconds((int)(maxTime));
}

void UBMageCharacterMovementComponent::HandleJump(UCameraComponent* activeCamera)
{
	if (IsAbilityActive(MovementAbilityType::Slide))
		TryStartAbility(MovementAbilityType::Launch);
	else if (CharacterOwner->bIsCrouched)
		CharacterOwner->UnCrouch();

	bool wallrunEnded = false;
	if (IsAbilityActive(MovementAbilityType::WallRun))
 		wallrunEnded = TryEndAbility(MovementAbilityType::WallRun);

	// movement redirection logic
	if (CharacterOwner->JumpCurrentCount < CharacterOwner->JumpMaxCount)
		RedirectVelocityToLookDirection(wallrunEnded, activeCamera);
}

void UBMageCharacterMovementComponent::RedirectVelocityToLookDirection(bool wallrunEnded, UCameraComponent* activeCamera)
{
	if (MovementMode != EMovementMode::MOVE_Falling)
		return;

	// jumping past apex will have the different gravity scale, reset it to the base
	if (GravityScale != CharacterBaseGravityScale)
		GravityScale = CharacterBaseGravityScale;

	// use the already normalized rotation vector as the base
	// NOTE: we are using the camera's rotation instead of the character's rotation to support wall running, which disables camera based character yaw control
	FVector targetDirectionVector = activeCamera->GetComponentRotation().Vector();
	//movement->GetLastUpdateRotation().Vector();

	// we only want to apply the movement input if it hasn't already been consumed
	if (ApplyMovementInputToJump && CharacterOwner->GetLastMovementInputVector() != FVector::ZeroVector && !wallrunEnded)
	{
		FVector movementImpact = CharacterOwner->GetLastMovementInputVector();
		targetDirectionVector += movementImpact;
	}

	// normalize both rotators to positive rotation so we can safely use them to calculate the yaw difference
	FRotator targetRotator = targetDirectionVector.Rotation();
	FRotator movementRotator = Velocity.Rotation();
	VectorMath::NormalizeRotator0To360(targetRotator);
	VectorMath::NormalizeRotator0To360(movementRotator);

	// calculate the yaw difference
	float yawDifference = targetRotator.Yaw - movementRotator.Yaw;

	// rotate the movement vector to the target direction
	Velocity = Velocity.RotateAngleAxis(yawDifference, FVector::ZAxisVector);
}

void UBMageCharacterMovementComponent::Server_RequestStartMovementAbility_Implementation(AActor* OtherActor, MovementAbilityType AbilityType, FMovementEventData MovementEventData)
{ 
	if (!OtherActor || !IsValid(OtherActor))
		return;

	if (!MovementAbilities.Contains(AbilityType))
		return;
	
	if (AbilityType == MovementAbilityType::WallRun)
	{
		// ShouldAbilityBegin's override for Wall Run needs to know what wall we're trying to run on
		auto Ability = Cast<UWallRunAbility>(MovementAbilities[AbilityType]);
		Ability->WallRunObject = OtherActor;
	}

	if (!ShouldAbilityBegin(AbilityType))
		return;
	
	// no parkour on other players
	if (OtherActor->IsA(APawn::StaticClass()))
		return;
	
	UAbilitySystemComponent* ASC = Cast<UAbilitySystemComponent>(CharacterOwner->GetComponentByClass(UAbilitySystemComponent::StaticClass()));
	if (!ASC)
		return;
	
	// Build GameplayEvent payload with the overlapped actor as the target
	FGameplayEventData EventData;
	EventData.Instigator = CharacterOwner;
	EventData.Target = OtherActor;
	EventData.EventTag = FGameplayTag::RequestGameplayTag(FName(TEXT("Movement.WallRun")));
	
	// Deliver the event so abilities bound to the tag receive the actor
	ASC->HandleGameplayEvent(EventData.EventTag, &EventData);
}

bool UBMageCharacterMovementComponent::Server_RequestStartMovementAbility_Validate(AActor* OtherActor, MovementAbilityType AbilityType, FMovementEventData MovementEventData)
{
	// basic quick validation (more checks below on server)
	return true;
}

void UBMageCharacterMovementComponent::TickGravityOverTime(float DeltaTime)
{
	if (_gravityCurveElapsed >= _gravityCurveDuration)
	{
		_activeGravityCurve = nullptr;
		_gravityCurveElapsed = milliseconds::zero();
		GravityScale = _initialGravityScale;
		return;
	}

	_gravityCurveElapsed += milliseconds((int)(DeltaTime * 1000.f));
	GravityScale = _activeGravityCurve->GetFloatValue(_gravityCurveElapsed.count());
}

void UBMageCharacterMovementComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	// Some abilities have an impact on the target's gravity
	if (_activeGravityCurve)
		TickGravityOverTime(DeltaTime);

	for (auto MovementAbilityTuple : MovementAbilities)
	{
		if (auto MovementAbility = Cast<UMovementAbility>(MovementAbilityTuple.Value); MovementAbility->ISMAActive())
			MovementAbility->Tick(DeltaTime);
	}
	
	// TODO: determine these based on priority
	if (MovementAbilities.Contains(MovementAbilityType::Slide) && MovementAbilities[MovementAbilityType::Slide]->ISMAActive())
	{
		MovementAbilities[MovementAbilityType::Slide]->Tick(DeltaTime);
		if (MovementAbilities[MovementAbilityType::Slide]->ShouldTransitionOut()) {
			// determine the appropriate speed cap
			// TODO: Figure out how to determine whether player wants to crouch at end of slide
			float speedLimit = MovementAbilities[MovementAbilityType::Sprint]->IsGAActive() ? SprintSpeed() : WalkSpeed;
			auto slideAbility = Cast<USlideAbility>(MovementAbilities[MovementAbilityType::Slide]);
			// This is kind of a hack but we're still crouched during the transition out, so we need to set the crouched speed
			MaxWalkSpeedCrouched = FMath::Min(MaxWalkSpeedCrouched + slideAbility->TransitionOutAccelertaionRate*DeltaTime, speedLimit);
		}
	}

	// if we're past the apex, apply the falling gravity scale
	// ignore this clause if we're wall running
	if (PreviousVelocity.Z >= 0.0f && Velocity.Z < -0.00001f && (!MovementAbilities.Contains(MovementAbilityType::Slide) || !MovementAbilities[MovementAbilityType::WallRun]->ISMAActive()))
		// this is undone in OnMovementModeChanged when the character lands
		GravityScale = CharacterPastJumpApexGravityScale;

	// TODO: check if the system is already tracking this
	PreviousVelocity = Velocity;

	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

UMovementAbility* UBMageCharacterMovementComponent::TryStartAbility(MovementAbilityType AbilityType)
{	
	// if (CharacterOwner && !CharacterOwner->IsLocallyControlled())
	// {
	// 	// only the client should be starting abilities directly
	// 	UE_LOG(LogTemp, Warning, TEXT("UBMageCharacterMovementComponent::TryStartAbility called on server"));
	// 	return nullptr;
	// }
	
	if (!MovementAbilities.Contains(AbilityType))
		return nullptr;
	
	TObjectPtr<UMovementAbility> Ability = MovementAbilities[AbilityType];	
	
	if (Ability->IsGAActive())
	{
		if (!Ability->ISMAActive())
			Ability->Begin(FMovementEventData());
		
		// if (const auto Controller = Cast<ABattlemageTheEndlessPlayerController>(GetController()))
		// {
		// 	const FString EnumName = StaticEnum<MovementAbilityType>()->GetNameStringByValue(static_cast<int64>(Ability->Type));
		// 	const FString LookupString = FString::Printf(TEXT("Movement.%s"), *EnumName);
		// 	Controller->Server_HandleMovementEvent(FGameplayTag::RequestGameplayTag(FName(LookupString)), CharacterOwner->GetLastMovementInputVector());
		// }
		
		return Ability;
	}
	
	return nullptr;
}

UMovementAbility* UBMageCharacterMovementComponent::TryStartAbilityFromEvent(MovementAbilityType AbilityType,
	const FGameplayEventData TriggerEventData)
{
	// only the client should be running this code, the server uses an RPC to handle activation
	if (!CharacterOwner->IsLocallyControlled())
	{		
		UE_LOG(LogTemp, Warning, TEXT("UBMageCharacterMovementComponent::TryStartAbilityFromEvent called on client"));
		return nullptr;
	}
	
	if (!MovementAbilities.Contains(AbilityType))
		return nullptr;
	
	TObjectPtr<UMovementAbility> ability = MovementAbilities[AbilityType];	
	
	if (ability->IsGAActive())
	{
		if (!ability->ISMAActive())
			ability->Begin(FMovementEventData());
		return ability;
	}
	
	return nullptr;
}

// TODO: Remove this after full migration
bool UBMageCharacterMovementComponent::ShouldAbilityBegin(MovementAbilityType abilityType)
{
	// we check what we can at the ability level
	if (!MovementAbilities.Contains(abilityType) || !MovementAbilities[abilityType]->ShouldBegin())
		return false;

	// handle ability interactions at this level
	if (!IsAbilityActive(MovementAbilityType::Sprint) && (abilityType == MovementAbilityType::WallRun || abilityType == MovementAbilityType::Launch))
		return false;

	return true;
}

bool UBMageCharacterMovementComponent::TryEndAbility(MovementAbilityType abilityType)
{
	if (!MovementAbilities.Contains(abilityType) || !IsAbilityActive(abilityType) || !MovementAbilities[abilityType]->ShouldEnd())
		return false;

	// handle ability interactions
	// This can be called before the timer goes off, so check if we're actually wall running
	if (abilityType == MovementAbilityType::WallRun)
	{
		if (GravityScale == CharacterBaseGravityScale)
			return false;

		AirControl = BaseAirControl;
	}
	else if (abilityType == MovementAbilityType::Sprint) {
		MaxWalkSpeed = WalkSpeed;
	}

	MovementAbilities[abilityType]->End();
	return true;
}

void UBMageCharacterMovementComponent::ForceEndAbility(MovementAbilityType abilityType)
{
	if (!IsAbilityActive(abilityType))
		return;

	MovementAbilities[abilityType]->End(true);
}

bool UBMageCharacterMovementComponent::IsAbilityActive(FGameplayTag abilityTag)
{
	FGameplayTagContainer ownedTags;
	CharacterOwner->FindComponentByClass<UAbilitySystemComponent>()->GetOwnedGameplayTags(ownedTags);

	return ownedTags.HasTag(abilityTag);
}

// TODO: Update this with begin/end calls instead of having to iterate every tick
MovementAbilityType UBMageCharacterMovementComponent::MostImportantActiveAbilityType()
{ 
	return MostImportantActiveAbility != nullptr ? MostImportantActiveAbility->Type : MovementAbilityType::None;
}

bool UBMageCharacterMovementComponent::IsWallRunToLeft()
{
	// this cast is technically unsafe but we know it is alright as this is explicitly init'd in the constructor
	return Cast<UWallRunAbility>(MovementAbilities[MovementAbilityType::WallRun])->WallIsToLeft;
}

void UBMageCharacterMovementComponent::OnMovementAbilityBegin(UMovementAbility* MovementAbility)
{
	// check if this ability should be the new most important, lower numerical value means higher priority
	if (MostImportantActiveAbility == nullptr || MostImportantActiveAbility->Priority > MovementAbility->Priority)
		MostImportantActiveAbility = MovementAbility;

	// handle interactions between abilities
	if (UDodgeAbility* dodgeAbility = Cast<UDodgeAbility>(MovementAbility)) 
	{
		// If we're sliding, end it before dodging
		TryEndAbility(MovementAbilityType::Slide);

		// If we're crouched, uncrouch before dodging
		if (CharacterOwner->bIsCrouched)
			CharacterOwner->UnCrouch();
	}
	else if (ULaunchAbility* launchAbility = Cast<ULaunchAbility>(MovementAbility))
	{
		LaunchesPerformed += 1;
	}
	else if (USlideAbility* slideAbility = Cast<USlideAbility>(MovementAbility))
	{
		// default walk is 1200, crouch is 300
		MaxWalkSpeedCrouched = SprintSpeed();
	}
	else if (USprintAbility* sprintAbility = Cast<USprintAbility>(MovementAbility))
	{
		ForceEndAbility(MovementAbilityType::Slide);
	}
	else if (UWallRunAbility* wallRunAbility = Cast<UWallRunAbility>(MovementAbility))
	{
		TryEndAbility(MovementAbilityType::Launch);
	}
	else if (UVaultAbility* vaultAbility = Cast<UVaultAbility>(MovementAbility))
	{
		// end every other ability, vault is greedy
		for (auto& a : MovementAbilities)
		{
			if (a.Value->IsGAActive() && a.Value != MovementAbility)
				TryEndAbility(a.Key);
		}
	}
}

void UBMageCharacterMovementComponent::OnMovementAbilityEnd(UMovementAbility* ability)
{
	// if the ability that just ended is not the most important, do nothing
	if (MostImportantActiveAbility != ability)
		return;

	// otherwise, find the next most important ability
	MostImportantActiveAbility = nullptr;
	for (auto& a : MovementAbilities)
	{
		if (a.Value->ISMAActive() && (MostImportantActiveAbility == nullptr || a.Value->Priority > MostImportantActiveAbility->Priority))
			MostImportantActiveAbility = a.Value;
	}

	if (USlideAbility* slideAbility = Cast<USlideAbility>(ability))
	{
		MaxWalkSpeedCrouched = CrouchSpeed;
		SetCrouchedHalfHeight(DefaultCrouchedHalfHeight);
		FRotator rotation = CharacterOwner->GetActorRotation();
		if (FMath::Abs(rotation.Pitch) > 0.0001f)
		{
			rotation.Pitch = 0;
			CharacterOwner->SetActorRotation(rotation);
		}
		if (!bWantsToCrouch)
		{
			UnCrouch();
			ShouldUnCrouch = false;
		}
		//auto targetAsc = CharacterOwner->FindComponentByClass<UAbilitySystemComponent>();
		//if (!targetAsc)
		//	return;
	}
}

void UBMageCharacterMovementComponent::RequestUnCrouch()
{
	if (!IsCrouching())
		return;

	ShouldUnCrouch = true;
}

void UBMageCharacterMovementComponent::OnMovementAbilityShouldTransitionOut(UMovementAbility* ability)
{
	if (USlideAbility* slideAbility = Cast<USlideAbility>(ability))
	{
		// this is toggled by control press and release
		if (!bWantsToCrouch)
			RequestUnCrouch();
	}
}

void UBMageCharacterMovementComponent::OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode)
{
	Super::OnMovementModeChanged(PreviousMovementMode, PreviousCustomMode);

	// this is in case of falling off an object while already overlapping a wallrunable object
	// if (PreviousMovementMode == EMovementMode::MOVE_Walking && MovementMode == EMovementMode::MOVE_Falling)
	// {
	// 	TryStartAbility(MovementAbilityType::WallRun);
	// }
	if (PreviousMovementMode == EMovementMode::MOVE_Falling && MovementMode == EMovementMode::MOVE_Walking)
	{
		TryEndAbility(MovementAbilityType::Launch);
		LaunchesPerformed = 0;
	}
}

void UBMageCharacterMovementComponent::SetVaultFootPlanted(bool value)
{
	Cast<UVaultAbility>(MovementAbilities[MovementAbilityType::Vault])->VaultFootPlanted = value;
}

float UBMageCharacterMovementComponent::SprintSpeed() const
{
	return Cast<USprintAbility>(MovementAbilities[MovementAbilityType::Sprint])->SprintSpeed;
}

void UBMageCharacterMovementComponent::ResetWalkSpeed()
{
	MaxWalkSpeed = IsAbilityActive(MovementAbilityType::Sprint) ? SprintSpeed() : WalkSpeed;
}

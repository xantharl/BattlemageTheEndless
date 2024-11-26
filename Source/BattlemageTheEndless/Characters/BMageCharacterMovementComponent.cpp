// Fill out your copyright notice in the Description page of Project Settings.


#include "BMageCharacterMovementComponent.h"

UBMageCharacterMovementComponent::UBMageCharacterMovementComponent()
{
	MovementAbilities.Add(MovementAbilityType::WallRun, CreateDefaultSubobject<UWallRunAbility>(TEXT("WallRun")));
	MovementAbilities.Add(MovementAbilityType::Launch, CreateDefaultSubobject<ULaunchAbility>(TEXT("Launch")));
	MovementAbilities.Add(MovementAbilityType::Sprint, CreateDefaultSubobject<USprintAbility>(TEXT("Sprint")));
	MovementAbilities.Add(MovementAbilityType::Slide, CreateDefaultSubobject<USlideAbility>(TEXT("Slide")));
	MovementAbilities.Add(MovementAbilityType::Vault, CreateDefaultSubobject<UVaultAbility>(TEXT("Vault")));
	MovementAbilities.Add(MovementAbilityType::Dodge, CreateDefaultSubobject<UDodgeAbility>(TEXT("Dodge")));

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

void UBMageCharacterMovementComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	// TODO: determine these based on priority
	if (IsAbilityActive(MovementAbilityType::Slide))
		MovementAbilities[MovementAbilityType::Slide]->Tick(DeltaTime);

	if (IsAbilityActive(MovementAbilityType::Launch))
		MovementAbilities[MovementAbilityType::Launch]->Tick(DeltaTime);

	if (IsAbilityActive(MovementAbilityType::Vault))
		MovementAbilities[MovementAbilityType::Vault]->Tick(DeltaTime);

	if (IsAbilityActive(MovementAbilityType::WallRun))
		MovementAbilities[MovementAbilityType::WallRun]->Tick(DeltaTime);

	// if we're past the apex, apply the falling gravity scale
	// ignore this clause if we're wall runninx
	if (PreviousVelocity.Z >= 0.0f && Velocity.Z < -0.00001f && !IsAbilityActive(MovementAbilityType::WallRun))
		// this is undone in OnMovementModeChanged when the character lands
		GravityScale = CharacterPastJumpApexGravityScale;

	// TODO: check if the system is already tracking this
	PreviousVelocity = Velocity;

	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

bool UBMageCharacterMovementComponent::TryStartAbility(MovementAbilityType abilityType)
{
	UMovementAbility* ability = MovementAbilities[abilityType];
	if (!ability->IsEnabled || ability->IsActive || !ShouldAbilityBegin(abilityType))
		return false;

	// handle ability interactions
	if (abilityType == MovementAbilityType::Sprint && IsCrouching())
		CharacterOwner->UnCrouch();
	if (abilityType == MovementAbilityType::Sprint && IsAbilityActive(MovementAbilityType::Slide))
		ForceEndAbility(MovementAbilityType::Slide);
	else if (abilityType == MovementAbilityType::Launch)
	{
		CharacterOwner->UnCrouch();		
		TryEndAbility(MovementAbilityType::Slide);

		// If we can't launch anymore, redirect to jump logic (which checks jump count and handles appropriately)
		if (LaunchesPerformed >= MaxLaunches)
		{
			CharacterOwner->Jump();
			return false;
		}
	}

	ability->Begin();
	return true;
}

bool UBMageCharacterMovementComponent::ShouldAbilityBegin(MovementAbilityType abilityType)
{
	// we check what we can at the ability level
	if (!MovementAbilities[abilityType]->ShouldBegin())
		return false;

	// handle ability interactions at this level
	if (!IsAbilityActive(MovementAbilityType::Sprint) && (abilityType == MovementAbilityType::WallRun || abilityType == MovementAbilityType::Launch))
		return false;

	return true;
}

bool UBMageCharacterMovementComponent::TryEndAbility(MovementAbilityType abilityType)
{
	if (!IsAbilityActive(abilityType) || !MovementAbilities[abilityType]->ShouldEnd())
		return false;

	// handle abiltiy interactions
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
			if (a.Value->IsActive && a.Value != MovementAbility)
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
		if (a.Value->IsActive && (MostImportantActiveAbility == nullptr || a.Value->Priority > MostImportantActiveAbility->Priority))
			MostImportantActiveAbility = a.Value;
	}

	if (USlideAbility* slideAbility = Cast<USlideAbility>(ability))
	{
		MaxWalkSpeedCrouched = IsCrouching() ? CrouchSpeed : WalkSpeed;
		SetCrouchedHalfHeight(DefaultCrouchedHalfHeight);
		FRotator rotation = CharacterOwner->GetActorRotation();
		if (FMath::Abs(rotation.Pitch) > 0.0001f)
		{
			rotation.Pitch = 0;
			CharacterOwner->SetActorRotation(rotation);
		}
	}
}

void UBMageCharacterMovementComponent::OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode)
{
	Super::OnMovementModeChanged(PreviousMovementMode, PreviousCustomMode);

	// this is in case of falling off an object while already overlapping a wallrunable object
	if (PreviousMovementMode == EMovementMode::MOVE_Walking && MovementMode == EMovementMode::MOVE_Falling)
	{
		TryStartAbility(MovementAbilityType::WallRun);
	}
	else if (PreviousMovementMode == EMovementMode::MOVE_Falling && MovementMode == EMovementMode::MOVE_Walking)
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

// Fill out your copyright notice in the Description page of Project Settings.


#include "BMageCharacterMovementComponent.h"

UBMageCharacterMovementComponent::UBMageCharacterMovementComponent()
{
	MovementAbilities.Add(MovementAbilityType::WallRun, CreateDefaultSubobject<UWallRunAbility>(TEXT("WallRun")));
	MovementAbilities.Add(MovementAbilityType::Launch, CreateDefaultSubobject<ULaunchAbility>(TEXT("Launch")));
	MovementAbilities.Add(MovementAbilityType::Slide, CreateDefaultSubobject<USlideAbility>(TEXT("Slide")));
	MovementAbilities.Add(MovementAbilityType::Vault, CreateDefaultSubobject<UVaultAbility>(TEXT("Vault")));

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
	if (!ability->IsEnabled || !ability->ShouldBegin())
		return false;

	ability->Begin();
	return true;
}

bool UBMageCharacterMovementComponent::TryEndAbility(MovementAbilityType abilityType)
{
	if (!IsAbilityActive(abilityType) || MovementAbilities[abilityType]->ShouldEnd())
		return false;

	// This can be called before the timer goes off, so check if we're actually wall running
	if (abilityType == MovementAbilityType::WallRun)
	{
		if (GravityScale == CharacterBaseGravityScale)
			return false;

		AirControl = BaseAirControl;
	}

	MovementAbilities[abilityType]->End();
	return true;
}

void UBMageCharacterMovementComponent::ForceEndAbility(MovementAbilityType abilityType)
{
	if (!IsAbilityActive(abilityType))
		return;

	MovementAbilities[abilityType]->End();
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
	// if there's a more imporant active ability do nothing
	if (MostImportantActiveAbility != nullptr && MostImportantActiveAbility->Priority <= MovementAbility->Priority)
		return;

	// otherwise set this ability as the most important
	MostImportantActiveAbility = MovementAbility;
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
}

void UBMageCharacterMovementComponent::OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode)
{
	Super::OnMovementModeChanged(PreviousMovementMode, PreviousCustomMode);

	if (PreviousMovementMode == EMovementMode::MOVE_Walking && MovementMode == EMovementMode::MOVE_Falling)
	{
		TryStartAbility(MovementAbilityType::WallRun);
	}
}

void UBMageCharacterMovementComponent::SetVaultFootPlanted(bool value)
{
	Cast<UVaultAbility>(MovementAbilities[MovementAbilityType::Vault])->VaultFootPlanted = value;
}

// Fill out your copyright notice in the Description page of Project Settings.


#include "BMageCharacterMovementComponent.h"

UBMageCharacterMovementComponent::UBMageCharacterMovementComponent()
{
	MovementAbilities = map<MovementAbilityType, TObjectPtr<UMovementAbility>>();

	WallRunAbility = CreateDefaultSubobject<UWallRunAbility>(TEXT("WallRun"));
	LaunchAbility = CreateDefaultSubobject<ULaunchAbility>(TEXT("Launch"));
	SlideAbility = CreateDefaultSubobject<USlideAbility>(TEXT("Slide"));
	VaultAbility = CreateDefaultSubobject<UVaultAbility>(TEXT("Vault"));

	MovementAbilities[MovementAbilityType::WallRun] = WallRunAbility;
	MovementAbilities[MovementAbilityType::Launch] = LaunchAbility;
	MovementAbilities[MovementAbilityType::Slide] = SlideAbility;
	MovementAbilities[MovementAbilityType::Vault] = VaultAbility;

	AirControl = BaseAirControl;
	GetNavAgentPropertiesRef().bCanCrouch = true;
}

void UBMageCharacterMovementComponent::InitAbilities(AActor* Character, USkeletalMeshComponent* Mesh)
{
	for (auto& ability : MovementAbilities)
	{
		ability.second->Init(this, Character, Mesh);
	}
}

void UBMageCharacterMovementComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	// TODO: determine these based on priority
	if (IsAbilityActive(MovementAbilityType::Slide))
		MovementAbilities[MovementAbilityType::Slide]->Tick(DeltaTime);

	if (IsAbilityActive(MovementAbilityType::Vault))
		MovementAbilities[MovementAbilityType::Vault]->Tick(DeltaTime);

	if (IsAbilityActive(MovementAbilityType::WallRun))
		MovementAbilities[MovementAbilityType::WallRun]->Tick(DeltaTime);

	// if we're past the apex, apply the falling gravity scale
	// ignore this clause if we're wall running
	if (PreviousVelocity.Z >= 0.0f && Velocity.Z < -0.00001f && !IsAbilityActive(MovementAbilityType::WallRun))
		// this is undone in OnMovementModeChanged when the character lands
		GravityScale = CharacterPastJumpApexGravityScale;

	// TODO: check if the system is already tracking this
	PreviousVelocity = Velocity;
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

// TODO: Make this use priority
MovementAbilityType UBMageCharacterMovementComponent::MostImportantActiveAbilityType()
{ 
	for(auto& ability : MovementAbilities)
	{
		if (ability.second->IsActive)
			return ability.first;
	}

	return MovementAbilityType::None;
}

UMovementAbility* UBMageCharacterMovementComponent::MostImportantActiveAbility()
{
	for (auto& ability : MovementAbilities)
	{
		if (ability.second->IsActive)
			return ability.second;
	}
	return nullptr;
}

bool UBMageCharacterMovementComponent::IsWallRunToLeft()
{
	// this cast is technically unsafe but we know it is alright as this is explicitly init'd in the constructor
	return Cast<UWallRunAbility>(MovementAbilities[MovementAbilityType::WallRun])->WallIsToLeft;
}

// Copyright Epic Games, Inc. All Rights Reserved.

#include "TP_WeaponComponent.h"
#include "BattlemageTheEndlessProjectile.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "Kismet/GameplayStatics.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Abilities/GameplayAbility.h"
#include "../Characters/BattlemageTheEndlessCharacter.h"

// Sets default values for this component's properties
UTP_WeaponComponent::UTP_WeaponComponent()
{
	// Default offset from the character location for projectiles to spawn
	MuzzleOffset = FVector(10.0f, 0.0f, 10.0f);
}

TSubclassOf<UGameplayAbility> UTP_WeaponComponent::GetAbilityByAttackType(EAttackType AttackType)
{
	// if there's only 1 ability, return it
	if (GrantedAbilities.Num() == 1)
	{
		return GrantedAbilities[0];
	}

	// otherwise check ability tags for AttackType
	FString attackTypeName = *UEnum::GetDisplayValueAsText(AttackType).ToString();
	auto attackTypeTags = FGameplayTagContainer(FGameplayTag::RequestGameplayTag(FName("Weapons.AttackTypes."+ attackTypeName)));

	auto matches = GrantedAbilities.FilterByPredicate(
		[attackTypeTags](TSubclassOf<UGameplayAbility> ability) {
			return ability->GetDefaultObject<UGameplayAbility>()->AbilityTags.HasAny(attackTypeTags); });

	if (matches.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("No ability found for attack type %s. This is likely invalid configuration."), *attackTypeName);
		return nullptr;
	}
	else if (matches.Num() > 1)
	{
		UE_LOG(LogTemp, Warning, TEXT("More than one combo found for attack type %s. Using the first."), *attackTypeName);
	}

	return matches[0];
}

void UTP_WeaponComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
}

void UTP_WeaponComponent::RemoveContext(ACharacter* character)
{
	if (APlayerController* PlayerController = Cast<APlayerController>(character->GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->RemoveMappingContext(WeaponMappingContext);
		}
	}
}

// Deprecated in favor of combo manager
// TODO: remove this once the combo manager is fully implemented
void UTP_WeaponComponent::AddBindings(ACharacter* character, UAbilitySystemComponent* abilityComponent)
{
	if (!character)
	{
		return;
	}
	APlayerController* PlayerController = Cast<APlayerController>(character->GetController());
	UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerController->InputComponent);

	if (!PlayerController || !EnhancedInputComponent)
	{
		return;
	}

	auto bindableActions = TArray<FGameplayAbilitySpecHandle>();
	FName targetTag = SlotType == EquipSlot::Primary ? FName("Weapons.Attack") : FName("Spells.Types.Quick");
	auto tags = FGameplayTagContainer(FGameplayTag::RequestGameplayTag(targetTag));
	abilityComponent->FindAllAbilitiesWithTags(	bindableActions, tags, false);

	if (bindableActions.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("No quick spells found"));
		return;
	}

	// We are binding them all to the same action and relying on proper config of blocking tags to determine which one to use
	for(auto action : bindableActions)
	{
		EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Triggered, this, &UTP_WeaponComponent::BindAbilityActivate, action, abilityComponent);
	}
}

// Deprecated in favor of combo manager
// TODO: remove this once the combo manager is fully implemented
void UTP_WeaponComponent::BindAbilityActivate(FGameplayAbilitySpecHandle abilityHandle, UAbilitySystemComponent* abilityComponent)
{
	// TODO: make this less horrible
	// handle the combo state so we only invoke the current attack
	// get the combo number from the ability
	auto baseComboTag = FGameplayTag::RequestGameplayTag(FName("State.Combo"));
	auto comboAttackNumber = abilityComponent->GetTagCount(baseComboTag);

	auto ability = abilityComponent->FindAbilitySpecFromHandle(abilityHandle); 
	auto abilityComboTags = ability->Ability->AbilityTags.Filter(FGameplayTagContainer(baseComboTag));
	if (abilityComboTags.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("Ability has no State.Combo.# tag"));
		return;
	}
	int abilityComboNumber = FCString::Atoi(*abilityComboTags.First().GetTagName().ToString().Right(1));

	if (comboAttackNumber == abilityComboNumber-1)
		abilityComponent->TryActivateAbility(abilityHandle, true);
}

// This is called by the AnimNotify_Collision Blueprint
void UTP_WeaponComponent::OnAnimTraceHit(ACharacter* character, const FHitResult& Hit)
{
	// If we hit a Battlemage character, apply damage	
	if (!Hit.HitObjectHandle.DoesRepresentClass(ABattlemageTheEndlessCharacter::StaticClass()))
		return;
	
	ABattlemageTheEndlessCharacter* otherCharacter = Cast<ABattlemageTheEndlessCharacter>(Hit.HitObjectHandle.FetchActor());

	// If this character has already been hit by this stage of the combo, don't hit them again
	auto characterIter = LastHitCharacters.find(Cast<ABattlemageTheEndlessCharacter>(otherCharacter));
	if (characterIter != LastHitCharacters.end() && characterIter->second == ComboAttackNumber)
	{
		return;
	}

	// Stop hitting yourself
	if (otherCharacter == character)
	{
		return;
	}

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 1.50f, FColor::Yellow, FString::Printf(TEXT("Hit on ComboAttackNumber %i"), ComboAttackNumber));
	}

	LastHitCharacters.insert_or_assign(otherCharacter, ComboAttackNumber);
	float Damage = LightAttackDamage;
	otherCharacter->ApplyDamage(Damage);
}
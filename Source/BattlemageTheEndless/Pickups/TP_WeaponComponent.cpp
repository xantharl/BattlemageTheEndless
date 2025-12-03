#include "TP_WeaponComponent.h"
#include "BattlemageTheEndlessProjectile.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "Kismet/GameplayStatics.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Abilities/GameplayAbility.h"
#include "../Characters/BattlemageTheEndlessCharacter.h"
#include "../Abilities/AttackBaseGameplayAbility.h"

TArray<TSubclassOf<class UGameplayAbility>> UTP_WeaponComponent::GetGrantedAbilitiesForDisplay()
{
	return GrantedAbilities.FilterByPredicate(
		[](const TSubclassOf<UGameplayAbility>& entry) {
			auto cdo = entry->GetDefaultObject<UAttackBaseGameplayAbility>();
			return !cdo->HasComboTag() || cdo->IsFirstInCombo();
		}
	);
}

// Sets default values for this component's properties
UTP_WeaponComponent::UTP_WeaponComponent()
{
	// Default offset from the character location for projectiles to spawn
	MuzzleOffset = FVector(10.0f, 0.0f, 10.0f);
}

void UTP_WeaponComponent::BeginPlay()
{
	// this needs to be before any returns because we break the engine if we return without routing to the super
	Super::BeginPlay();
}

TSubclassOf<UGameplayAbility> UTP_WeaponComponent::GetAbilityByAttackType(EAttackType AttackType)
{
	// if there's only 1 ability, return it
	if (GrantedAbilities.Num() == 1)
		return GrantedAbilities[0];

	// otherwise check ability tags for AttackType
	FString attackTypeName = *UEnum::GetDisplayValueAsText(AttackType).ToString();
	auto attackTypeTag = FGameplayTag::RequestGameplayTag(FName("Weapon.AttackTypes."+ attackTypeName));

	auto matches = GrantedAbilities.FilterByPredicate(
		[attackTypeTag](TSubclassOf<UGameplayAbility> ability) {
			return Cast<UGameplayAbility>(ability->ClassDefaultObject)->AbilityTags.HasTag(attackTypeTag);
		});

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

void UTP_WeaponComponent::OnAbilityCancelled(const FAbilityEndedData& endData)
{
	ResetHits();
}

void UTP_WeaponComponent::ResetHits()
{
	LastHitCharacters.Empty();
	LastAttackAnimationName = FString();
}

void UTP_WeaponComponent::NextOrPreviousSpell(bool nextOrPrevious)
{
	if (SlotType != EquipSlot::Secondary)
	{
		UE_LOG(LogTemp, Warning, TEXT("Only secondary weapons can switch spells"));
		return;
	}

	// find the index of the current ActiveAbility in GrantedAbilities
	int ActiveAbilityIndex = GrantedAbilities.Find((TSubclassOf<UGameplayAbility>)SelectedAbility);
	if (ActiveAbilityIndex == INDEX_NONE)
	{
		UE_LOG(LogTemp, Warning, TEXT("ActiveAbility not found in GrantedAbilities"));
		return;
	}

	UAttackBaseGameplayAbility* abilityDefaultObject;
	// select the next or previous spell, wrapping around the array and continuing if the ability is part of a combo
	//	 and not the start of it
	do 
	{
		ActiveAbilityIndex = nextOrPrevious ? ActiveAbilityIndex + 1 : ActiveAbilityIndex - 1;
		if (ActiveAbilityIndex < 0)
		{
			ActiveAbilityIndex = GrantedAbilities.Num() - 1;
		}
		else if (ActiveAbilityIndex >= GrantedAbilities.Num())
		{
			ActiveAbilityIndex = 0;
		}
		SelectedAbility = GrantedAbilities[ActiveAbilityIndex];
		abilityDefaultObject = SelectedAbility->GetDefaultObject<UAttackBaseGameplayAbility>();
	} while (abilityDefaultObject->HasComboTag() && !abilityDefaultObject->IsFirstInCombo());

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 1.50f, FColor::Yellow, FString::Printf(TEXT("Switched to %s"), *SelectedAbility->GetName()));
	}
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

// This is called by the AnimNotify_Collision Blueprint
void UTP_WeaponComponent::OnAnimTraceHit(ACharacter* character, const FHitResult& Hit, FString attackAnimationName)
{
	if (GEngine)
		GEngine->AddOnScreenDebugMessage(-1, 1.50f, FColor::Blue, FString::Printf(TEXT("%s hit character %s"),
			*(character->GetName()), *(Hit.GetActor()->GetName())));

	if (attackAnimationName != LastAttackAnimationName)
	{
		ResetHits();
		LastAttackAnimationName = attackAnimationName;
	}

	if (!Hit.GetActor())
	{
		return;
	}

	// if either actor involed does not have an ASC, exit
	auto attackerAsc = character->FindComponentByClass<UBMageAbilitySystemComponent>();
	auto hitActor = Hit.GetActor();
	auto hitActorAsc = hitActor->FindComponentByClass<UAbilitySystemComponent>();

	// If this character has already been hit by this stage of the combo, don't hit them again
	if (!attackerAsc || !hitActorAsc || LastHitCharacters.Contains(hitActor) || hitActor == character)
	{
		return;
	}

	/*if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 1.50f, FColor::Yellow, FString::Printf(TEXT("Hit by Animation %s"), *LastAttackAnimationName));
	}*/

	LastHitCharacters.Add(hitActor);

	auto ability = attackerAsc->FindAbilitySpecFromHandle(attackerAsc->ComboManager->LastActivatedAbilityHandle);
	if (!ability)
	{
		UE_LOG(LogTemp, Error, TEXT("LastActivatedAbility Not found, if you hit this ActivateAbility was probably called directly, use UFUNCTION ProcessInputAndBindAbilityCancelled"));
		return;
	}
	// get the active ability
	auto abilitySpec = Cast<UAttackBaseGameplayAbility>(
		attackerAsc->FindAbilitySpecFromHandle(attackerAsc->ComboManager->LastActivatedAbilityHandle)->Ability);

	// apply any on hit effects from the weapon attack, all effects on a weapon are assumed to be on hit
	abilitySpec->ApplyEffects(hitActor, hitActorAsc, character);
}
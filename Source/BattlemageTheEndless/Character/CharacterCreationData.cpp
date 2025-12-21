// Fill out your copyright notice in the Description page of Project Settings.


#include "CharacterCreationData.h"

#include "BattlemageTheEndless/Helpers/WidgetHelperFunctions.h"
//
void UCharacterCreationData::BuildSelectedPickupActors(const class UWorld* World)
{
	// This is pretty gross but GetAllActorsOfClass doesn't support templated types
	TArray<AActor*> AllPickupActors;
	UGameplayStatics::GetAllActorsOfClass(World, APickupActor::StaticClass(), AllPickupActors);
	TArray<APickupActor*> PickupActors;
	for (AActor* Actor : AllPickupActors)
	{
		if (APickupActor* PickupActor = Cast<APickupActor>(Actor))
		{
			PickupActors.Add(PickupActor);
		}
	}
	
	for (auto SpellClassName : SelectedArchetypeNames)
	{
		SelectedPickupActors.Add(
			UWidgetHelperFunctions::GetPickupBySpellClassName(FText::FromString(SpellClassName), 
			                                                  PickupActors)->GetClass());
	}
	
	// Saving this for character portion of impl
	// // Make sure we preserve melee weapons since there isn't a selection for them yet
	// auto CurrentMeleeWeapons = Character->Equipment[EquipSlot::Primary].Pickups;
	//
	// Character->DefaultEquipment = SelectedPickupClasses;
	// for (auto meleeWeapon : CurrentMeleeWeapons)
	// {
	// 	Character->DefaultEquipment.Add(meleeWeapon->GetClass());
	// }
}

void UCharacterCreationData::BuildSelectedSpells(class UWorld* World)
{
	// Use Selected Pickup Actors to find spell abilities since the initial selection options are based on pickups
	for (const auto PickupActor : SelectedPickupActors)
	{
		const auto CDO = PickupActor->GetDefaultObject<APickupActor>();
		if (CDO->PickupType != EPickupType::SpellClass)
			continue;
		
		for (auto AbilityClass : CDO->Weapon->GrantedAbilities)
		{
			if (SelectedSpellNames.Contains(UWidgetHelperFunctions::GetAbilityFriendlyName(AbilityClass).ToString()))
			{
				SelectedSpells.Add(AbilityClass);
			}
		}
	}
}

void UCharacterCreationData::InitSelections(const TArray<FString> ArchetypeNames, const TArray<FString> SpellNames)
{
	const auto World = GetWorld();
	if (!World)
	{
		UE_LOG(LogTemp, Error, TEXT("Could not get world in InitSelections"));
		return;
	}
	
	SelectedArchetypeNames = ArchetypeNames;
	SelectedSpellNames = SpellNames;
	
	BuildSelectedPickupActors(World);
	BuildSelectedSpells(World);
}

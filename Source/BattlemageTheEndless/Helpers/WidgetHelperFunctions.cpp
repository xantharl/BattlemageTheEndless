// Fill out your copyright notice in the Description page of Project Settings.


#include "WidgetHelperFunctions.h"

#include "BattlemageTheEndless/Abilities/GA_WithEffectsBase.h"

APickupActor* UWidgetHelperFunctions::GetPickupBySpellClassName(
	const FText SpellClassName, TArray<APickupActor*> AllPickups)
{
	auto RetVal = TArray<TSubclassOf<UGameplayAbility>>();

	FString TagString = FString::Printf(TEXT("Spells.%s"), *SpellClassName.ToString());
	FGameplayTag SearchTag = FGameplayTag::RequestGameplayTag(FName(*TagString));
	
	for (APickupActor* a : AllPickups)
	{
		// Making a possibly dangerous assumption that the pickup's first tag will be the spell class tag
		if (a->PickupType != EPickupType::SpellClass)
			continue;
		
		if (a->GrantedTags.HasTag(SearchTag))
			return a;
	}
	
	return nullptr;
}

FName UWidgetHelperFunctions::GetAbilityFriendlyName(TSubclassOf<UGameplayAbility> AbilityClass)
{
	FString RetVal;
	
	if (!AbilityClass)
		return FName("Invalid Ability");

	UGameplayAbility* AbilityCDO = AbilityClass->GetDefaultObject<UGameplayAbility>();
	if (!AbilityCDO)
		return FName("Invalid Ability");

	if (const auto AsEffectsBase = Cast<UGA_WithEffectsBase>(AbilityCDO); RetVal.IsEmpty() && AsEffectsBase)
		RetVal = AsEffectsBase->GetAbilityName().ToString();

	if (RetVal.IsEmpty() && AbilityCDO->GetAssetTags().Num() > 0)
		RetVal = AbilityCDO->GetAssetTags().GetByIndex(0).GetTagLeafName().ToString();
	
	if (RetVal.IsEmpty())
		RetVal = AbilityCDO->GetName();
	
	return FName(FName::NameToDisplayString(RetVal, false));
}

const FGameplayTagContainer& UWidgetHelperFunctions::GetAbilityTags(UGameplayAbility* Ability)
{
	return Ability->GetAssetTags();
}

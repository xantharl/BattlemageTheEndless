// Fill out your copyright notice in the Description page of Project Settings.


#include "OnHitSoundData.h"

UOnHitSoundData::UOnHitSoundData()
{
}

UOnHitSoundData::~UOnHitSoundData()
{
}

USoundBase* UOnHitSoundData::GetHitSoundForMaterialAndAbility(UPhysicalMaterial* PhysicalMaterial,UGameplayAbility* Ability)
{
	const auto MatchedProperties = HitSoundMaterialProperties.FindByPredicate([this,PhysicalMaterial,Ability](const FHitSoundMaterialProperties& Properties)
	{
		bool Match = PhysicalMaterial == nullptr || PhysicalMaterial == Properties.PhysicalMaterial;
		if (!Ability)
			Match &= Properties.RequiredAbilityTags.IsEmpty();
		else
			Match &= Ability->AbilityTags.HasAll(Properties.RequiredAbilityTags);		
		return Match;
	});
	
	if (!MatchedProperties)
		return nullptr;
	return MatchedProperties->HitSound;
}

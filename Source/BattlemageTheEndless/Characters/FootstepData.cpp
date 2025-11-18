// Fill out your copyright notice in the Description page of Project Settings.


#include "FootstepData.h"

UFootstepData::UFootstepData()
{
}

UFootstepData::~UFootstepData()
{
}

USoundCue* UFootstepData::GetFootstepSoundForMaterialAndSpeed(UPhysicalMaterial* PhysicalMaterial, float MovementSpeed)
{
	if (FootstepMaterialProperties.Num() == 0) // We aren't checking for nullptr PhysicalMaterial here, as that is how we're handling default case.
	{
		UE_LOG(LogTemp, Warning, TEXT("FootstepMaterialProperties is empty."));
		return nullptr;
	}

	if (!_hasBeenSorted)
	{
		// sort each FootstepSounds map by key (movement speed)
		for (auto& props : FootstepMaterialProperties)
		{
			props.FootstepSounds.KeySort(TLess<float>());
		}
		_hasBeenSorted = true;
	}

	for (auto props : FootstepMaterialProperties)
	{
		if (props.PhysicalMaterial != PhysicalMaterial)
			continue;

		USoundCue* selectedSound = nullptr;
		for (TPair<float, USoundCue*> speedSoundPair : props.FootstepSounds)
		{
			if (speedSoundPair.Key > MovementSpeed)
				return selectedSound;

			selectedSound = speedSoundPair.Value;
		}

		// default case: return the last sound in the map if we're going faster than any defined speed
		return selectedSound;
	}

	return nullptr;
}

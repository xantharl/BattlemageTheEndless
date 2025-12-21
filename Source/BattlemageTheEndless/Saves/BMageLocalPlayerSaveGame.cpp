// Fill out your copyright notice in the Description page of Project Settings.


#include "BMageLocalPlayerSaveGame.h"

void UBMageLocalPlayerSaveGame::InitSaveData(ABattlemageTheEndlessCharacter* InCharacter)
{
	if (InCharacter->CharacterCreationData)
	{
		// Create a new UObject copy so our original doesn't go out of scope
		CharacterCreationData = DuplicateObject<UCharacterCreationData>(InCharacter->CharacterCreationData, this);		
		SetSaveSlotName(CharacterCreationData->CharacterName);
		PlayerTransform = InCharacter->GetActorTransform();
	}
}

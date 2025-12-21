// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BattlemageTheEndless/Character/CharacterCreationData.h"
#include "BattlemageTheEndless/Characters/BattlemageTheEndlessCharacter.h"
#include "GameFramework/SaveGame.h"
#include "BMageLocalPlayerSaveGame.generated.h"

/**
 * 
 */
UCLASS()
class BATTLEMAGETHEENDLESS_API UBMageLocalPlayerSaveGame : public ULocalPlayerSaveGame
{
	GENERATED_BODY()
	
public:
	/** Character creation data selected by the player at the start of the game, Use InitSaveData to set this **/
	UPROPERTY(BlueprintReadOnly, Category = "SaveData", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCharacterCreationData> CharacterCreationData;

	UFUNCTION(BlueprintCallable, Category = "SaveData", meta = (AllowPrivateAccess = "true"))
	void InitSaveData(ABattlemageTheEndlessCharacter* InCharacter);
	
	/** Transform of the player when the game was last saved **/
	UPROPERTY(BlueprintReadWrite, Category = "SaveData", meta = (AllowPrivateAccess = "true"))
	FTransform PlayerTransform; 
};

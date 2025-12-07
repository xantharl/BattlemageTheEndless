// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "../Characters/BattlemageTheEndlessCharacter.h"
#include "BattlemageTheEndless/Saves/BMageLocalPlayerSaveGame.h"
#include "UObject/ConstructorHelpers.h"
#include "BattlemageTheEndlessGameMode.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlayerDiedSignature, ACharacter*, Character);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnGameSaved, const FString&, SlotName, const int32, UserIndex, bool, bSuccess);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnGameLoaded, const FString&, SlotName, const int32, UserIndex, USaveGame*, LoadedGameData);
UCLASS(minimalapi)

class ABattlemageTheEndlessGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ABattlemageTheEndlessGameMode();

	UPROPERTY(BlueprintAssignable, Category = "GameMode|Delegates", meta = (AllowPrivateAccess = "true"))
	FOnPlayerDiedSignature OnPlayerDied;
	UPROPERTY(BlueprintAssignable, Category = "GameMode|Delegates", meta = (AllowPrivateAccess = "true"))
	FOnGameSaved OnGameSaved;
	UPROPERTY(BlueprintAssignable, Category = "GameMode|Delegates", meta = (AllowPrivateAccess = "true"))
	FOnGameLoaded OnGameLoaded;
	
    //Tries to Spawn the player's pawn.
    virtual void RestartPlayer(AController* NewPlayer) override;
	
	UFUNCTION(BlueprintCallable, Category = "SaveGame", meta = (AllowPrivateAccess = "true"))
	void OnGameSavedDelegateFunction(const FString& SlotName, const int32 UserIndex, bool bSuccess);

	UFUNCTION(BlueprintCallable, Category = "SaveGame", meta = (AllowPrivateAccess = "true"))
	void OnGameLoadedDelegateFunction(const FString& SlotName, const int32 UserIndex, USaveGame* LoadedGameData);
	
	UFUNCTION(BlueprintCallable, Category = "SaveGame", meta = (AllowPrivateAccess = "true"))	
	void SaveGame(int PlayerIndex = 0);
	
	UFUNCTION(BlueprintCallable, Category = "SaveGame", meta = (AllowPrivateAccess = "true"))
	void LoadGame(FString SlotName, int PlayerIndex = 0);

protected:
    virtual void BeginPlay() override;

    //Called when Player character has died.
    UFUNCTION()
    virtual void PlayerDied(ACharacter* Character);
};




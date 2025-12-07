// Copyright Epic Games, Inc. All Rights Reserved.

#include "BattlemageTheEndlessGameMode.h"

ABattlemageTheEndlessGameMode::ABattlemageTheEndlessGameMode()
	: Super()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnClassFinder(TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"));
	DefaultPawnClass = PlayerPawnClassFinder.Class;

}

void ABattlemageTheEndlessGameMode::BeginPlay()
{
    Super::BeginPlay();

    //Bind our Player died delegate to the Gamemode's PlayerDied function.
    if (!OnPlayerDied.IsBound())
    {
        OnPlayerDied.AddDynamic(this, &ABattlemageTheEndlessGameMode::PlayerDied);
    }
}

void ABattlemageTheEndlessGameMode::RestartPlayer(AController* NewPlayer)
{
    Super::RestartPlayer(NewPlayer);
}

void ABattlemageTheEndlessGameMode::OnGameSavedDelegateFunction(const FString& SlotName, const int32 UserIndex,
    bool bSuccess)
{
    OnGameSaved.Broadcast(SlotName, UserIndex, bSuccess);
}

void ABattlemageTheEndlessGameMode::OnGameLoadedDelegateFunction(const FString& SlotName, const int32 UserIndex,
    USaveGame* LoadedGameData)
{
    OnGameLoaded.Broadcast(SlotName, UserIndex, LoadedGameData);
}

void ABattlemageTheEndlessGameMode::SaveGame(int PlayerIndex)
{
    APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), PlayerIndex);
    if (!PC)
    {
        UE_LOG(LogTemp, Error, TEXT("No player controller found, cannot save"));
        return;
    }
    auto Instance = GetGameInstance();
    if (!Instance)
    {
        UE_LOG(LogTemp, Error, TEXT("No game instance found, cannot save"));
        return;
    }
    
    ABattlemageTheEndlessCharacter* Character = Cast<ABattlemageTheEndlessCharacter>(PC->GetPawn());
    if (!Character)
    {
        UE_LOG(LogTemp, Error, TEXT("No character found, cannot save"));
        return;
    }
    
    if (UBMageLocalPlayerSaveGame* SaveGameInstance = Cast<UBMageLocalPlayerSaveGame>(UGameplayStatics::CreateSaveGameObject(UBMageLocalPlayerSaveGame::StaticClass())))
    {
        // Set up the (optional) delegate.
        FAsyncSaveGameToSlotDelegate SavedDelegate;
        // USomeUObjectClass::SaveGameDelegateFunction is a void function that takes the following parameters: const FString& SlotName, const int32 UserIndex, bool bSuccess
        SavedDelegate.BindUObject(this, &ABattlemageTheEndlessGameMode::OnGameSavedDelegateFunction);
	 
        // Set data on the savegame object.
        SaveGameInstance->InitSaveData(Character);
	 
        // Start async save process.
        UGameplayStatics::AsyncSaveGameToSlot(SaveGameInstance, SaveGameInstance->GetSaveSlotName(), PlayerIndex, SavedDelegate);
    }
}

void ABattlemageTheEndlessGameMode::LoadGame(FString SlotName, int PlayerIndex)
{
    // Set up the delegate.
    FAsyncLoadGameFromSlotDelegate LoadedDelegate;
    // USomeUObjectClass::LoadGameDelegateFunction is a void function that takes the following parameters: const FString& SlotName, const int32 UserIndex, USaveGame* LoadedGameData
    LoadedDelegate.BindUObject(this, &ABattlemageTheEndlessGameMode::OnGameLoadedDelegateFunction);
    UGameplayStatics::AsyncLoadGameFromSlot(SlotName, PlayerIndex, LoadedDelegate);
}

void ABattlemageTheEndlessGameMode::PlayerDied(ACharacter* Character)
{
    //Get a reference to our Character's Player Controller
    AController* CharacterController = Character->GetController();
    RestartPlayer(CharacterController);
}

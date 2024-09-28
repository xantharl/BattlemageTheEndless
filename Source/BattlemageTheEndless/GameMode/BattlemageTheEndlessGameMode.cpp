// Copyright Epic Games, Inc. All Rights Reserved.

#include "BattlemageTheEndlessGameMode.h"
#include "../Characters/BattlemageTheEndlessCharacter.h"
#include "UObject/ConstructorHelpers.h"

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

// TODO: figure out how to keep items on death
void ABattlemageTheEndlessGameMode::RestartPlayer(AController* NewPlayer)
{
    Super::RestartPlayer(NewPlayer);
}

void ABattlemageTheEndlessGameMode::PlayerDied(ACharacter* Character)
{
    //Get a reference to our Character's Player Controller
    AController* CharacterController = Character->GetController();
    RestartPlayer(CharacterController);
}
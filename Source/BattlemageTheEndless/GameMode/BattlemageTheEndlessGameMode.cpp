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
// Copyright Epic Games, Inc. All Rights Reserved.

#include "Portal2GameMode.h"
#include "Portal2Character.h"
#include "UObject/ConstructorHelpers.h"

APortal2GameMode::APortal2GameMode()
	: Super()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnClassFinder(TEXT("/Game/FirstPerson/Blueprints/BP_FirstPersonCharacter"));
	DefaultPawnClass = PlayerPawnClassFinder.Class;

}

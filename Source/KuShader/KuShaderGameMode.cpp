// Copyright Epic Games, Inc. All Rights Reserved.

#include "KuShaderGameMode.h"
#include "KuShaderCharacter.h"
#include "UObject/ConstructorHelpers.h"

AKuShaderGameMode::AKuShaderGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}

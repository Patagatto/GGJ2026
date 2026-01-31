// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/GGJGamemode.h"

#include "UObject/ConstructorHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "Characters/GGJCharacter.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "GameFramework/PlayerController.h"

AGGJGamemode::AGGJGamemode()
{
	TotalScore = 0;

	// --- HARDCODED PAWN CLASS ---
	// This is the definitive fix to ensure the correct Blueprint character is always spawned for all players,
	// bypassing any potential conflicting settings in the editor.
	// IMPORTANT: Make sure the path below matches the reference path of your character Blueprint.
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/Blueprints/Characters/BP_GGJCharacter"));
	if (PlayerPawnBPClass.Class != nullptr)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("CRITICAL ERROR: Could not find BP_GGJCharacter at the specified path in GGJGamemode.cpp. Please check the path."));
	}
}


void AGGJGamemode::AddScore(int32 Amount)
{
	TotalScore += Amount;
}

void AGGJGamemode::StartSplitScreenSession()
{
	// Ensure we have a valid world context before proceeding.
	UWorld* World = GetWorld();
	if (!World) return;

	// 1. Check if the second player (Controller ID 1) already exists to prevent creating duplicates.
	if (UGameplayStatics::GetPlayerController(World, 1))
	{
		UE_LOG(LogTemp, Warning, TEXT("StartSplitScreenSession: Player 1 already exists."));
		return;
	}

	// 2. Get the GameInstance, which is responsible for managing local players.
	UGameInstance* GameInstance = GetGameInstance();
	if (!GameInstance)
	{
		UE_LOG(LogTemp, Error, TEXT("StartSplitScreenSession: Failed to get GameInstance."));
		return;
	}

	// 3. Create the second local player with Controller ID 1. The 'true' flag also spawns their controller.
	FString ErrorMessage;
	if (ULocalPlayer* NewPlayer = GameInstance->CreateLocalPlayer(1, ErrorMessage, true))
	{
		UE_LOG(LogTemp, Log, TEXT("StartSplitScreenSession: Successfully created Player 1."));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("StartSplitScreenSession: Failed to create Player 1. Error: %s"), *ErrorMessage);
	}
}

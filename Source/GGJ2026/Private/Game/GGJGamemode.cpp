// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/GGJGamemode.h"

#include "UObject/ConstructorHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "Engine/GameViewportClient.h"
#include "Camera/SharedCamera.h"

AGGJGamemode::AGGJGamemode()
{
	TotalScore = 0;
	
}

void AGGJGamemode::BeginPlay()
{
	Super::BeginPlay();

	// Assign Shared Camera to the primary player on start
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
	{
		AssignSharedCamera(PC);
	}
}

void AGGJGamemode::AddScore(int32 Amount)
{
	TotalScore += Amount;
}

void AGGJGamemode::InitializeSecondPlayer()
{
	UWorld* World = GetWorld();
	if (!World) return;

	// Prevent duplicate creation of Player 1
	if (UGameplayStatics::GetPlayerController(World, 1))
	{
		UE_LOG(LogTemp, Warning, TEXT("StartSplitScreenSession: Player 1 already exists."));
		return;
	}

	// Retrieve GameInstance for local player management
	UGameInstance* GameInstance = GetGameInstance();
	if (!GameInstance)
	{
		UE_LOG(LogTemp, Error, TEXT("StartSplitScreenSession: Failed to get GameInstance."));
		return;
	}

	// Create Player 1 (Controller ID 1). 'true' spawns the controller immediately.
	FString ErrorMessage;
	if (ULocalPlayer* NewPlayer = GameInstance->CreateLocalPlayer(1, ErrorMessage, true))
	{
		UE_LOG(LogTemp, Log, TEXT("StartSplitScreenSession: Successfully created Player 1."));
		AssignSharedCamera(NewPlayer->GetPlayerController(GetWorld()));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("StartSplitScreenSession: Failed to create Player 1. Error: %s"), *ErrorMessage);
	}
}

void AGGJGamemode::AssignSharedCamera(APlayerController* Controller)
{
	if (!Controller) return;

	// Locate the Shared Camera actor in the level
	if (AActor* SharedCam = UGameplayStatics::GetActorOfClass(this, ASharedCamera::StaticClass()))
	{
		Controller->SetViewTargetWithBlend(SharedCam);
		UE_LOG(LogTemp, Log, TEXT("AssignSharedCamera: Assigned SharedCamera to %s"), *Controller->GetName());
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("AssignSharedCamera: No ASharedCamera found in the level!"));
	}
}

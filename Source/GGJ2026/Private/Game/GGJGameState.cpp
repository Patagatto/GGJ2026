// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/GGJGameState.h"

void AGGJGameState::BeginPlay()
{
	Super::BeginPlay();
	
	InitializeGame();
}

void AGGJGameState::InitializeGame()
{
	CurrentGameTime = 0.0f;
	// Ensure any existing timer is cleared before starting a new one
	GetWorld()->GetTimerManager().ClearTimer(GameTimeHandle);
	GetWorld()->GetTimerManager().SetTimer(GameTimeHandle,this, &AGGJGameState::UpdateGameTime, 1.0f, true);
}

void AGGJGameState::UpdateGameTime()
{
	CurrentGameTime += 1.0f;

	if (CurrentGameTime >= MaxGameTime)
	{
		GetWorld()->GetTimerManager().ClearTimer(GameTimeHandle);
		OnGameTimeEnded();
	}
}

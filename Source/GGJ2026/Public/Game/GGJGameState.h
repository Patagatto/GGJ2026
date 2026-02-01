// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "GGJGameState.generated.h"

/**
 * 
 */
UCLASS(Abstract)
class GGJ2026_API AGGJGameState : public AGameStateBase
{
	GENERATED_BODY()
public:
	
	virtual void BeginPlay() override;
	FTimerHandle GameTimeHandle;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Time")
	float CurrentGameTime = 0;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time")
	float MaxGameTime = 180.0f;
	
	UFUNCTION()
	void InitializeGame();
	
	UFUNCTION()
	void UpdateGameTime();
	
	/** Event called when the game time reaches MaxGameTime. Implement this in Blueprint. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Game Flow")
	void OnGameTimeEnded();
	
};

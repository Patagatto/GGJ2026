// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GGJGamemode.generated.h"

/**
 * 
 */
UCLASS(Abstract)
class GGJ2026_API AGGJGamemode : public AGameModeBase
{
	GENERATED_BODY()
	
public:
	AGGJGamemode();
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Score")
	int32 TotalScore;
	
	UFUNCTION(BlueprintCallable, Category = "Score")
	void AddScore(int32 Amount);

	/** Initializes the second local player for the session. */
	UFUNCTION(BlueprintCallable, Category = "Player")
	void InitializeSecondPlayer();

	/** Checks if any players remain alive. Triggers OnAllPlayersDead if none found. */
	UFUNCTION(BlueprintCallable, Category = "Game Flow")
	void CheckPlayerStatus();

	/** Event called when all players are dead (Game Over). */
	UFUNCTION(BlueprintImplementableEvent, Category = "Game Flow")
	void OnAllPlayersDead();
	
protected:
	virtual void BeginPlay() override;
	
private:	
	void AssignSharedCamera(APlayerController* Controller);
};

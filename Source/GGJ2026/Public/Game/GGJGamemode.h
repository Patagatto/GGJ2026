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

	/** Creates the second local player and enables split-screen. Call this from Blueprint to start the session. */
	UFUNCTION(BlueprintCallable, Category = "Player")
	void StartSplitScreenSession();
};

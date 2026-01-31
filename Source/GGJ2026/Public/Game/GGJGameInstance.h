// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "UI/MainMenuWidget.h"
#include "GGJGameInstance.generated.h"

/**
 * 
 */
UCLASS()
class GGJ2026_API UGGJGameInstance : public UGameInstance
{
	GENERATED_BODY()
	
protected:
	EPlayMode PlayMode;
	
public:
	UFUNCTION(BlueprintCallable)
	void SetPlayMode(EPlayMode NewMode);
	
	UFUNCTION(BlueprintCallable)
	EPlayMode GetPlayMode() const;
	
	
};

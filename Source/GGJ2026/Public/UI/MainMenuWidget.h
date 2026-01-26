// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "MainMenuWidget.generated.h"

/**
 * 
 */
UCLASS()
class GGJ2026_API UMainMenuWidget : public UUserWidget
{
	GENERATED_BODY()
	
protected:
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	UImage* GameTitle;
	
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	UImage* BackGround;
	
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	UButton* StartButton;
	
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	UButton* OptionsButton;
	
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	UButton* QuitButton;
	
	UPROPERTY(EditDefaultsOnly)
	FName StartLevelName;
	
	virtual void NativeConstruct() override;

public:
	
	UFUNCTION(BlueprintCallable)
	void StartGame();
	
	void QuitGame();
};

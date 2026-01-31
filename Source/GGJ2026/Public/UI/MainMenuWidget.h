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
enum class EPlayMode : uint8
{
	SinglePlayer,
	MultiPlayer,
};

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
	UButton* StartButton1P;
	
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	UButton* StartButton2P;
	
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	UButton* OptionsButton;
	
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	UButton* QuitButton;
	
	UPROPERTY(EditDefaultsOnly)
	FName StartLevelName;
	
	EPlayMode Mode;
	
	virtual void NativeConstruct() override;

public:
	UFUNCTION(BlueprintCallable)
	void StartGame1P();
	
	UFUNCTION(BlueprintCallable)
	void StartGame2P();
	
	UFUNCTION(BlueprintCallable)
	void QuitGame();
};

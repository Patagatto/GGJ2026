// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/StackBox.h"
#include "MainMenuWidget.generated.h"

/**
 * 
 */
UENUM(BlueprintType)
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
	// Images
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	UImage* GameTitle;
	
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	UImage* BackGround;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	UStackBox* WarningBox;
	
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	UImage* WarningBG;
	
	// Buttons
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	UButton* StartButton1P;
	
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	UButton* StartButton2P;
	
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	UButton* OptionsButton;
	
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	UButton* QuitButton;
	
	// Variables		
	UPROPERTY(EditDefaultsOnly)
	FName StartLevelName;
	
	// Animations
	UPROPERTY(Transient, meta = (BindWidgetAnim))
	UWidgetAnimation* WarningAnimation;
	
	UPROPERTY(Transient, meta = (BindWidgetAnim))
	UWidgetAnimation* FadeAnimation;
	
	EPlayMode Mode;
	
	virtual void NativeConstruct() override;

public:
	UFUNCTION(BlueprintCallable)
	void StartGame1P();
	
	UFUNCTION(BlueprintCallable)
	void StartGame2P();
	
	UFUNCTION(BlueprintCallable)
	void QuitGame();
	
	UFUNCTION(BlueprintCallable)
	void LoadLevel();
};

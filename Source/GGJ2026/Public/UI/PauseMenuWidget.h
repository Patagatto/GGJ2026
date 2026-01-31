// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "PauseMenuWidget.generated.h"

/**
 * 
 */
UCLASS()
class GGJ2026_API UPauseMenuWidget : public UUserWidget
{
	GENERATED_BODY()
	
protected:
	// Buttons
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	UButton* ContinueButton;
		
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	UButton* OptionsButton;
	
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	UButton* MenuButton;
	
	// Variables		
	UPROPERTY(EditDefaultsOnly)
	FName MenuLevelName;
	
	// Animations
	UPROPERTY(Transient, meta = (BindWidgetAnim))
	UWidgetAnimation* FadeAnimation;
	
public:
	virtual void NativeConstruct() override;
	
	UFUNCTION(BlueprintCallable)
	void ResumeGame();
	
	UFUNCTION(BlueprintCallable)
	void BackToMenu();
};

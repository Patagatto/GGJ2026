// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Border.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "TextBox.generated.h"

/**
 * 
 */
UCLASS()
class GGJ2026_API UTextBoxWidget : public UUserWidget
{
	GENERATED_BODY()
	
protected:
	//Canvas properties
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	UBorder* BoxBackgroundImg;
	
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	UImage* ContinueImg;
	
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	UTextBlock* LineText;
	
	//Programming properties	
	UPROPERTY(EditDefaultsOnly)
	int32 CurrentIndex;
	
	UPROPERTY(EditDefaultsOnly)
	float DelayBetweenText;
	
	UPROPERTY(EditDefaultsOnly)
	FString CurrentString;
	
	UPROPERTY(EditDefaultsOnly)
	TArray<FString> CharArray;
	
	UPROPERTY(EditDefaultsOnly)
	FTimerHandle TimerHandle;
	
	virtual void NativeConstruct() override;

public:
	UPROPERTY(EditDefaultsOnly)
	bool IsFillingText;
	
	UFUNCTION(BlueprintCallable)
	void SkipDialogueText();
	
	UFUNCTION(BlueprintCallable)
	void InitDialogueText(const FString& Text);
	
	UFUNCTION(BlueprintCallable)
	void TickDialogueText();
		
};

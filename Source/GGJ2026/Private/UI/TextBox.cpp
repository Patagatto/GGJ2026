// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/TextBox.h"
#include "Kismet/KismetStringLibrary.h"
#include "Kismet/KismetSystemLibrary.h"

void UTextBoxWidget::NativeConstruct()
{
	Super::NativeConstruct();
}

void UTextBoxWidget::SkipDialogueText()
{
	IsFillingText = false;
	CurrentString = "";
		
	GetWorld()->GetTimerManager().ClearTimer(TimerHandle);
	
	for (const FString& Char : CharArray)
	{
		CurrentString += Char;
	}
	
	LineText->SetText(FText::FromString(CurrentString));
}

void UTextBoxWidget::InitDialogueText(const FString& Text)
{
	CharArray = UKismetStringLibrary::GetCharacterArrayFromString(Text);
	
	CurrentIndex = 0;
	CurrentString = "";
	
	GetWorld()->GetTimerManager().SetTimer(TimerHandle, this, &UTextBoxWidget::TickDialogueText, DelayBetweenText, true);
}

void UTextBoxWidget::TickDialogueText()
{
	IsFillingText = true;
	CurrentString += CharArray[CurrentIndex++];
	
	LineText->SetText(FText::FromString(CurrentString));
	
	if (CurrentIndex >= CharArray.Num())
	{
		IsFillingText = false;
		GetWorld()->GetTimerManager().ClearTimer(TimerHandle);
	}
}
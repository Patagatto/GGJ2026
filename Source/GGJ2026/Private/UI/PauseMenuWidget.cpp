// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/PauseMenuWidget.h"

#include "Kismet/GameplayStatics.h"

void UPauseMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();
	
	ContinueButton->OnClicked.AddDynamic(this, &UPauseMenuWidget::ResumeGame);
	MenuButton->OnClicked.AddDynamic(this, &UPauseMenuWidget::BackToMenu);
}

void UPauseMenuWidget::ResumeGame()
{
	SetVisibility(ESlateVisibility::Hidden);
	UGameplayStatics::SetGamePaused(GetWorld(), false);
}

void UPauseMenuWidget::BackToMenu()
{
	PlayAnimation(FadeAnimation);
	// FadeAnimation->BindToAnimationFinished(this, );
	UGameplayStatics::OpenLevel(GetWorld(), MenuLevelName);
}

// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/MainMenuWidget.h"

#include "Kismet/GameplayStatics.h"
#include "Runtime/Engine/Internal/Kismet/BlueprintTypeConversions.h"

void UMainMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();
	
	StartButton->OnClicked.AddDynamic(this, &UMainMenuWidget::StartGame);
	// StartButton->OnHovered.AddDynamic(this, );
}

void UMainMenuWidget::StartGame()
{
	if (StartLevelName == FName("E_FirstLevel"))
	{
		// Start fading animation
	
		UGameplayStatics::OpenLevel(GetWorld(), StartLevelName);
	}
}

void UMainMenuWidget::QuitGame()
{
	// Are you sure message
	
	UKismetSystemLibrary::QuitGame(GetWorld(), UGameplayStatics::GetPlayerController(GetWorld(), 0), EQuitPreference::Quit, true);
}

// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/MainMenuWidget.h"

#include "Game/GGJGameInstance.h"
#include "GameFramework/InputDeviceLibrary.h"
#include "Kismet/GameplayStatics.h"

void UMainMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();
	
	StartButton1P->OnClicked.AddDynamic(this, &UMainMenuWidget::StartGame1P);
	StartButton2P->OnClicked.AddDynamic(this, &UMainMenuWidget::StartGame2P);
	QuitButton->OnClicked.AddDynamic(this, &UMainMenuWidget::QuitGame);
}

void UMainMenuWidget::StartGame1P()
{
	if (StartLevelName == FName("E_MainLevel"))
	{
		// Start fading animation
		Cast<UGGJGameInstance>(GetGameInstance())->PlayMode = EPlayMode::SinglePlayer;
		UGameplayStatics::OpenLevel(GetWorld(), StartLevelName);
	}
}

void UMainMenuWidget::StartGame2P()
{
	if (StartLevelName == FName("E_MainLevel"))
	{
		// Start fading animation
		UInputDeviceLibrary::GetAllConnectedInputDevices();
		Cast<UGGJGameInstance>(GetGameInstance())->PlayMode = EPlayMode::MultiPlayer;
		UGameplayStatics::OpenLevel(GetWorld(), StartLevelName);
	}
}

void UMainMenuWidget::QuitGame()
{
	// Are you sure message
	UKismetSystemLibrary::QuitGame(GetWorld(), UGameplayStatics::GetPlayerController(GetWorld(), 0), EQuitPreference::Quit, true);
}

// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/MainMenuWidget.h"

#include "Animation/WidgetAnimation.h"
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
	}
}

void UMainMenuWidget::StartGame2P()
{
	if (StartLevelName == FName("E_MainLevel"))
	{
		// Check if two inputs are connected
		TArray<FInputDeviceId> Devices;
		if (UInputDeviceLibrary::GetAllConnectedInputDevices(Devices) < 2)
		{
			PlayAnimationForward(WarningAnimation);
		}
		else
		{
			PlayAnimationForward(FadeAnimation);
			// FadeAnimation->BindToAnimationFinished(this, );
			Cast<UGGJGameInstance>(GetGameInstance())->PlayMode = EPlayMode::MultiPlayer;
		}
	}
}

void UMainMenuWidget::QuitGame()
{
	// Are you sure message
	UKismetSystemLibrary::QuitGame(GetWorld(), UGameplayStatics::GetPlayerController(GetWorld(), 0), EQuitPreference::Quit, true);
}

void UMainMenuWidget::LoadLevel()
{
	UGameplayStatics::OpenLevel(GetWorld(), StartLevelName);
}

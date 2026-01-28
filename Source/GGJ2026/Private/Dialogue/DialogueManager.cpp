// Fill out your copyright notice in the Description page of Project Settings.


#include "Dialogue/DialogueManager.h"

#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "UI/TextBox.h"

// Sets default values
ADialogueManager::ADialogueManager()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
}

// Called when the game starts or when spawned
void ADialogueManager::BeginPlay()
{
	Super::BeginPlay();
	
	DialogueWidget = Cast<UWidgetComponent>(GetComponentByClass(UWidgetComponent::StaticClass()));
	
	if (DialogueWidget)
	{
		TextBoxWidget = Cast<UTextBoxWidget>(DialogueWidget->GetWidget());
	}
}

// Called every frame
void ADialogueManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void ADialogueManager::StartDialogue(AActor* InstigatorActor, const FSequenceStruct& SequenceStruct)
{
	if (DialogueWidget)
	{
		if (DialogueWidget->bHiddenInGame)
        {
            IsComplete = false;
            CurrentIndex = 0;
            CurrentInstigator = InstigatorActor;
            CurrentSequence = SequenceStruct;
            DialogueWidget->SetHiddenInGame(false);
            NextDialogue();
        }
	}
	
}

void ADialogueManager::NextDialogue()
{
	if (!IsComplete)
	{
		if (TextBoxWidget->IsFillingText)
		{
			TextBoxWidget->SkipDialogueText();
		}
		else
		{
			if (CurrentIndex >= CurrentSequence.TextLines.Num())
			{
				DialogueWidget->SetHiddenInGame(true);
				IsComplete = true;
			}
			else
			{
				//DialogueWidget->SetWorldLocationAndRotation(CurrentInstigator->GetActorLocation(), UKismetMathLibrary::FindLookAtRotation(CurrentInstigator->GetActorLocation(), UGameplayStatics::GetPlayerPawn(GetWorld(), 0)->GetActorLocation()));
				DialogueWidget->SetWorldLocation(CurrentInstigator->GetActorLocation());
				
				TextBoxWidget->InitDialogueText(CurrentSequence.TextLines[CurrentIndex]);
				CurrentIndex++;
			}
		}
	}
}
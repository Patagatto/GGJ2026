// Fill out your copyright notice in the Description page of Project Settings.


#include "Dialogue/DialogueManager.h"

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
	
	if (Widget)
	{
		TextBoxWidgetWidget = Cast<UTextBoxWidget>(Widget->GetWidget());
	}
}

// Called every frame
void ADialogueManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void ADialogueManager::StartDialogue(AActor* InstigatorActor, const FSequenceStruct& SequenceStruct)
{
	if (Widget->bHiddenInGame)
	{
		IsComplete = false;
		CurrentIndex = 0;
		CurrentInstigator = InstigatorActor;
		CurrentSequence = SequenceStruct;
		Widget->SetHiddenInGame(false);
		NextDialogue();
	}
}

void ADialogueManager::NextDialogue()
{
	if (!IsComplete)
	{
		if (TextBoxWidgetWidget->IsFillingText) //IS FILLING TEXT
		{
			//Skip Dialogue
		}
		else
		{
			if (true) //IS THE LAST DIALOGUE SEQUENCE
			{
				Widget->SetHiddenInGame(true);
				IsComplete = true;
			}
			else
			{
				// Set widget location and rotation, play pop up anim, set dialogue text
				CurrentIndex++;
			}
		}
	}
}
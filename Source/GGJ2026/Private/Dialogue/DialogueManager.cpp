// Fill out your copyright notice in the Description page of Project Settings.


#include "Dialogue/DialogueManager.h"

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
	
}

// Called every frame
void ADialogueManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ADialogueManager::StartDialogue()
{
	if (Widget->bHiddenInGame)
	{
		IsComplete = false;
		CurrentIndex = 0;
		//CurrentInstigator = Instigator;
		// Current Sequence
		Widget->SetHiddenInGame(false);
		NextDialogue();
	}
}

void ADialogueManager::NextDialogue()
{
	if (!IsComplete)
	{
		if (true) //IS FILLING TEXT
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
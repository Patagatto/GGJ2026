// Fill out your copyright notice in the Description page of Project Settings.


#include "Dialogue/DialogueActor.h"

#include "Kismet/GameplayStatics.h"

// Sets default values
ADialogueActor::ADialogueActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
}

// Called when the game starts or when spawned
void ADialogueActor::BeginPlay()
{
	Super::BeginPlay();
	
	DialogueManager = Cast<ADialogueManager>(UGameplayStatics::GetActorOfClass(GetWorld(), ADialogueManager::StaticClass()));
}

// Called every frame
void ADialogueActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void ADialogueActor::Interact()
{
	//DialogueManager->StartDialogue(this);
}
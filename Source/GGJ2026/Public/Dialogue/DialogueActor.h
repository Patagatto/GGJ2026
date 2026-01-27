// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "DialogueManager.h"
#include "InteractionInterface.h"
#include "GameFramework/Actor.h"
#include "DialogueActor.generated.h"

// This is a test class for the dialogue (it should be the final npc)
UCLASS()
class GGJ2026_API ADialogueActor : public AActor, public IInteractionInterface
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ADialogueActor();

protected:
	UPROPERTY()
	ADialogueManager* DialogueManager;
	
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	
	virtual void Interact() override;

};

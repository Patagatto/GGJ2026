// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/Actor.h"
#include "DialogueManager.generated.h"

UCLASS()
class GGJ2026_API ADialogueManager : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ADialogueManager();

protected:
	// Defines if all the dialogue sequences have been shown
	bool IsComplete = false;
	
	// Current sequence
	
	// Defines the current dialogue sequence
	int CurrentIndex;
	
	// The actor that called the function
	UPROPERTY()
	AActor* CurrentInstigator;
	
	UPROPERTY(EditDefaultsOnly)
	UWidgetComponent* Widget;
	
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	void StartDialogue();
	
	void NextDialogue();
};

// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SequenceStruct.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/Actor.h"
#include "UI/TextBox.h"
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
		
	// Defines the current dialogue sequence
	int32 CurrentIndex = 0;
	
	// Current sequence
	UPROPERTY()
	FSequenceStruct CurrentSequence;
	
	// The actor that called the function
	UPROPERTY()
	AActor* CurrentInstigator;
	
	UPROPERTY()
	UWidgetComponent* DialogueWidget;
	
	UPROPERTY()
	UTextBoxWidget* TextBoxWidget;
	
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	void StartDialogue(AActor* InstigatorActor, const FSequenceStruct& SequenceStruct);
	
	void NextDialogue();
};

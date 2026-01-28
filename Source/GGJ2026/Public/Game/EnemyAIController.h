// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "DetourCrowdAIController.h"
#include "EnemyAIController.generated.h"

/**
 * 
 */
UCLASS()
class GGJ2026_API AEnemyAIController : public ADetourCrowdAIController
{
	GENERATED_BODY()
	
protected:
	virtual void BeginPlay() override;
	
	virtual void OnPossess(APawn* InPawn) override;
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	UBehaviorTree* AIBehaviorTree;
};

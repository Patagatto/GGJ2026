// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/EnemyAIController.h"

#include "Kismet/GameplayStatics.h"

void AEnemyAIController::BeginPlay()
{
	Super::BeginPlay();
}

void AEnemyAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	
	if (AIBehaviorTree)
	{
		RunBehaviorTree(AIBehaviorTree);
	}
}
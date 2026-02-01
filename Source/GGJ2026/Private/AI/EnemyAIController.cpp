// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/EnemyAIController.h"

#include "BrainComponent.h"
#include "Navigation/CrowdFollowingComponent.h"

void AEnemyAIController::BeginPlay()
{
	Super::BeginPlay();

	if (UCrowdFollowingComponent* Crowd = FindComponentByClass<UCrowdFollowingComponent>())
	{
		Crowd->SetCrowdAvoidanceQuality(ECrowdAvoidanceQuality::Medium);
	}
}

void AEnemyAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
}

void AEnemyAIController::ActivateEnemyBT(bool IsEnemyReset)
{
	if (IsEnemyReset)
	{
		BrainComponent->RestartLogic();
	}
	else if (AIBehaviorTree)
	{
		RunBehaviorTree(AIBehaviorTree);
	}
}

void AEnemyAIController::DeactivateEnemyBT()
{
	BrainComponent->StopLogic("");
}

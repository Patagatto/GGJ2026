// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/EnemyAIController.h"

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
	
	if (AIBehaviorTree)
	{
		RunBehaviorTree(AIBehaviorTree);
			}
}

void AEnemyAIController::ActivateEnemyBT()
{
	if (AIBehaviorTree)
	{
		RunBehaviorTree(AIBehaviorTree);
	}
}

void AEnemyAIController::DeactivateEnemyBT()
{
	CleanupBrainComponent();
}

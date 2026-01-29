// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/EnemyManager.h"

void UEnemyAttackManager::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	
	ActiveTokenHolders.Empty();
}

void UEnemyAttackManager::SetMaxToken(int32 NewMax)
{
	MaxTokens = NewMax;
}

bool UEnemyAttackManager::HasToken(AActor* EnemyActor) const
{
	return ActiveTokenHolders.Contains(EnemyActor);
}

bool UEnemyAttackManager::RequestAttack(AActor* EnemyActor)
{
	if (!EnemyActor || (!HasToken(EnemyActor) && ActiveTokenHolders.Num() >= MaxTokens)) return false;
	
	if (!HasToken(EnemyActor))	ActiveTokenHolders.Add(EnemyActor);
	return true;
}

void UEnemyAttackManager::ReleaseToken(AActor* EnemyActor)
{
	if (EnemyActor)
	{
		if (HasToken(EnemyActor))
		{
			ActiveTokenHolders.Remove(EnemyActor);
			ActiveTokenHolders.Compact();
			ActiveTokenHolders.Shrink();
		}
	}
}

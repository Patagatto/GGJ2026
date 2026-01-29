// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/EnemyManager.h"

void UEnemyManager::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	
	UE_LOG(LogTemp, Error, TEXT("=== SUBSYSTEM INITIALIZED ==="));
	UE_LOG(LogTemp, Error, TEXT("World: %s"), *GetWorld()->GetMapName());
	
	ActiveTokenHolders.Empty();
}

bool UEnemyManager::HasToken(AActor* EnemyActor) const
{
	return ActiveTokenHolders.Contains(EnemyActor);
}

bool UEnemyManager::RequestAttack(AActor* EnemyActor)
{
	if (!EnemyActor || (!HasToken(EnemyActor) && ActiveTokenHolders.Num() >= MaxToken)) return false;
	
	if (!HasToken(EnemyActor))	ActiveTokenHolders.Add(EnemyActor);
	return true;
}

void UEnemyManager::ReleaseToken(const AActor* EnemyActor)
{
	if (EnemyActor)
	{
		ActiveTokenHolders.Remove(EnemyActor);
		ActiveTokenHolders.Compact();
		ActiveTokenHolders.Shrink();
	}
}

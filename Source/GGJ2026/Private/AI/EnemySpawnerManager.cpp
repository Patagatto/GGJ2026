// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/EnemySpawnerManager.h"

#include "Kismet/GameplayStatics.h"

void UEnemySpawnerManager::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	
	ActiveEnemies.Reserve(MaxActiveEnemies);
	EnemyPool.Empty();
}

void UEnemySpawnerManager::SetMaxEnemies(int32 NewMax)
{
	MaxEnemies = NewMax;
}

void UEnemySpawnerManager::SetMaxActiveEnemies(int32 NewMax)
{
	if (NewMax > MaxEnemies) NewMax = MaxEnemies;
	
	MaxActiveEnemies = NewMax;
}

void UEnemySpawnerManager::SetSpawnRate(float NewRate)
{
	SpawnRate = NewRate;
}

void UEnemySpawnerManager::AddEnemyToPool(AEnemyCharacter* Enemy)
{
	if (Enemy)
	{
		EnemyPool.Enqueue(Enemy);
	}
}

void UEnemySpawnerManager::InitSpawn()
{
	GetWorld()->GetTimerManager().SetTimer(SpawnTimer, this, &UEnemySpawnerManager::SpawnEnemy, SpawnRate, true);
}

void UEnemySpawnerManager::SpawnEnemy()
{
	if (!EnemyPool.IsEmpty())
	{
		AEnemyCharacter* CurrentEnemy;
		EnemyPool.Dequeue(CurrentEnemy);
		CurrentEnemy->ActivateEnemy();	
		ActiveEnemies.Add(CurrentEnemy);
	}
}

void UEnemySpawnerManager::ResetEnemy(AEnemyCharacter* Enemy)
{
	if (Enemy)
	{
		ActiveEnemies.Remove(Enemy);
		ActiveEnemies.Compact();
		ActiveEnemies.Shrink();
		
		Enemy->DeactivateEnemy();
		EnemyPool.Enqueue(Enemy);
	}
}
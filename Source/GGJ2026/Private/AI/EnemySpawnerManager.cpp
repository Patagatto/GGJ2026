// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/EnemySpawnerManager.h"

#include "Game/EnemySpawner.h"
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

void UEnemySpawnerManager::SetEnemyClass(UClass* NewClass)
{
	EnemyClass = NewClass;
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
	TArray<AActor*> Spawners;
	TArray<AEnemyCharacter*> Enemies;
	TArray<EEnemyType> TypeList;
	
	int32 CountType0 = FMath::RoundToInt(MaxEnemies * 0.40f);
	int32 CountType1 = FMath::RoundToInt(MaxEnemies * 0.20f);
	int32 CountType2 = FMath::RoundToInt(MaxEnemies * 0.20f);
    int32 CountType3 = MaxEnemies - (CountType0 + CountType1 + CountType2);
	
	for (int32 i = 0; i < CountType0; i++) TypeList.Add(EEnemyType::Maskless);
	for (int32 i = 0; i < CountType1; i++) TypeList.Add(EEnemyType::Rabbit);
	for (int32 i = 0; i < CountType2; i++) TypeList.Add(EEnemyType::Bird);
	for (int32 i = 0; i < CountType3; i++) TypeList.Add(EEnemyType::Cat);
	
	int32 CurrentTypeIndex = 0;
	
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AEnemySpawner::StaticClass(), Spawners);
	
	if (EnemyClass && Spawners.Num() > 0)
	{
		int32 BaseAmount = MaxEnemies / Spawners.Num();
		int32 Extra = MaxEnemies % Spawners.Num();

		for (int32 i = 0; i < Spawners.Num(); ++i)
		{
			int32 EnemiesInSpawner = BaseAmount + (i < Extra ? 1 : 0);
    
			for (int32 j = 0; j < EnemiesInSpawner; ++j)
			{
				AEnemyCharacter* Enemy = Cast<AEnemyCharacter>(GetWorld()->SpawnActor(EnemyClass));
				Enemy->SpawnLocation = Cast<AEnemySpawner>(Spawners[i])->GetSpawnLocation();
				Enemy->SetActorLocation(Enemy->SpawnLocation);
				Enemy->Type = TypeList[CurrentTypeIndex++];
				Enemies.Add(Enemy);				
			}
		}
		
		int32 LastIndex = Enemies.Num() - 1;
		for (int32 i = 0; i <= LastIndex; ++i)
		{
			int32 Index = FMath::RandRange(i, LastIndex);
			if (i != Index) Enemies.Swap(i, Index);
		}

		for (AEnemyCharacter* Enemy : Enemies)
		{
			EnemyPool.Enqueue(Enemy);
		}
	}
}

void UEnemySpawnerManager::SetSpawnTimer()
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
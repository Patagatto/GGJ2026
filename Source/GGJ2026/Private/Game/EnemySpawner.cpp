// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/EnemySpawner.h"

#include "AI/EnemySpawnerManager.h"

// Sets default values
AEnemySpawner::AEnemySpawner()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	
}

// Called when the game starts or when spawned
void AEnemySpawner::BeginPlay()
{
	Super::BeginPlay();
	
	UEnemySpawnerManager* SpawnerManager = GetWorld()->GetSubsystem<UEnemySpawnerManager>();
	SpawnerManager->SetMaxEnemies(MaxEnemies);
	SpawnerManager->SetMaxActiveEnemies(MaxActiveEnemies);
	
	if (EnemyClass)
	{
		for (int i = 0; i < MaxEnemies; i++)
     	{
			AEnemyCharacter* Enemy = Cast<AEnemyCharacter>(GetWorld()->SpawnActor(EnemyClass));
			SpawnerManager->AddEnemyToPool(Enemy);
     	}
	}
}

// Called every frame
void AEnemySpawner::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}


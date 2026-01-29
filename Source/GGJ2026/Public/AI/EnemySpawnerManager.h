// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Characters/EnemyCharacter.h"
#include "Subsystems/WorldSubsystem.h"
#include "EnemySpawnerManager.generated.h"

/**
 * 
 */
UCLASS()
class GGJ2026_API UEnemySpawnerManager : public UWorldSubsystem
{
	GENERATED_BODY()
	
protected:
	UPROPERTY()
	int32 MaxEnemies = 50;
	
	UPROPERTY()
	int32 MaxActiveEnemies = 30;
	
	UPROPERTY(EditAnywhere)
	float SpawnRate = 2.0f;
	
	FTimerHandle SpawnTimer;
		
	TQueue<AEnemyCharacter*> EnemyPool;
	
	UPROPERTY()
	TSet<AEnemyCharacter*> ActiveEnemies;
		
	
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	
	UFUNCTION(BlueprintCallable)
	void SetMaxEnemies(int32 NewMax);
	
	UFUNCTION(BlueprintCallable)
	void SetMaxActiveEnemies(int32 NewMax);
		
	UFUNCTION(BlueprintCallable)
	void SetSpawnRate(float NewRate);
	
	void AddEnemyToPool(AEnemyCharacter* Enemy);
	
	UFUNCTION(BlueprintCallable)
	void InitSpawn();
	
	void SpawnEnemy();
	
	void ResetEnemy(AEnemyCharacter* Enemy);
};

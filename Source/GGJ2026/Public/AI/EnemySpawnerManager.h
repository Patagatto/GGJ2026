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
	int32 MaxEnemies = 40;
	
	UPROPERTY()
	int32 MaxActiveEnemies = 20;
	
	UPROPERTY(EditAnywhere)
	float SpawnRate = 5.0f;
	
	UPROPERTY(EditAnywhere)
	UClass* EnemyClass;
	
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
	
	UFUNCTION(BlueprintCallable)
	void SetEnemyClass(UClass* NewClass);
	
	void AddEnemyToPool(AEnemyCharacter* Enemy);
	
	UFUNCTION(BlueprintCallable)
	void InitSpawn();
		
	UFUNCTION(BlueprintCallable)
	void SetSpawnTimer();
	
	void SpawnEnemy();
	
	void ResetEnemy(AEnemyCharacter* Enemy);
};

// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "EnemyManager.generated.h"

/**
 * 
 */
UCLASS()
class GGJ2026_API UEnemyManager : public UWorldSubsystem
{
	GENERATED_BODY()
		
protected:
	UPROPERTY(EditAnywhere)
	int32 MaxToken = 2;
	
	UPROPERTY()
	TSet<AActor*> ActiveTokenHolders;
	
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	
	// Attack Token Handling
	UFUNCTION(BlueprintCallable)
	bool HasToken(AActor* EnemyActor) const;
	
	UFUNCTION(BlueprintCallable)
	bool RequestAttack(AActor* EnemyActor);
	
	UFUNCTION(BlueprintCallable)
	void ReleaseToken(const AActor* EnemyActor);
};

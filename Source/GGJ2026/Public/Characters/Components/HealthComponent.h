// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HealthComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class GGJ2026_API UHealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UHealthComponent();

protected:
	UPROPERTY(EditDefaultsOnly)
	float MaxHealth;
	
	UPROPERTY(BlueprintReadOnly)
	float CurrentHealth;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool IsDead;
		
	// Called when the game starts
	virtual void BeginPlay() override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool IsHit;
	
	UFUNCTION(BlueprintCallable)
	void ApplyDamage(float Damage);
	
	UFUNCTION(BlueprintCallable)
	bool IsActorDead() const;
	
	UFUNCTION(BlueprintCallable)
	void Reset();
	
	UFUNCTION(BlueprintCallable)
	void FinishHit();
	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

		
};

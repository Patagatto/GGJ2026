// Fill out your copyright notice in the Description page of Project Settings.


#include "Characters/Components/HealthComponent.h"

// Sets default values for this component's properties
UHealthComponent::UHealthComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;
}


// Called when the game starts
void UHealthComponent::BeginPlay()
{
	Super::BeginPlay();
	
	Reset();
}

void UHealthComponent::ApplyDamage(float Damage)
{
	CurrentHealth -= Damage;
	IsHit = true;
	
	if (CurrentHealth <= 0.0f) IsDead = true;
	else if (CurrentHealth > MaxHealth) CurrentHealth = MaxHealth;
}

bool UHealthComponent::IsActorDead() const
{
	return IsDead;
}

void UHealthComponent::Reset()
{
	CurrentHealth = MaxHealth;
	IsDead = false;
}

void UHealthComponent::FinishHit()
{
	IsHit = false;
}


// Called every frame
void UHealthComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}


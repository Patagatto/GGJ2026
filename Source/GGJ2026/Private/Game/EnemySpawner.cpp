// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/EnemySpawner.h"

#include "Kismet/KismetMathLibrary.h"

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
}

FVector AEnemySpawner::GetSpawnLocation()
{
	FVector RandomLocation = UKismetMathLibrary::RandomPointInBoundingBox(GetActorLocation(), FVector(BoxHalfSize, BoxHalfSize, BoxHalfSize));
	RandomLocation.Z = GetActorLocation().Z;
	return RandomLocation;
}

// Called every frame
void AEnemySpawner::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}
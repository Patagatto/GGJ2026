// Fill out your copyright notice in the Description page of Project Settings.


#include "Characters/EnemyCharacter.h"

#include "Kismet/GameplayStatics.h"

// Sets default values
AEnemyCharacter::AEnemyCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	
	Box = CreateDefaultSubobject<UBoxComponent>(FName("Box"));
	Box->OnComponentBeginOverlap.AddDynamic(this, &AEnemyCharacter::OnBoxBeginOverlap);
	
	HealthComp = CreateDefaultSubobject<UHealthComponent>(FName("Health"));
}

// Called when the game starts or when spawned
void AEnemyCharacter::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AEnemyCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AEnemyCharacter::OnBoxBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (OtherActor == UGameplayStatics::GetPlayerPawn(GetWorld(), 0))
	{
		if (UHealthComponent* Health = Cast<UHealthComponent>(OtherActor->GetComponentByClass(UHealthComponent::StaticClass()))) Health->ApplyDamage(Damage);
	}
}

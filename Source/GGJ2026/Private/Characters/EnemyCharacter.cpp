// Fill out your copyright notice in the Description page of Project Settings.


#include "Characters/EnemyCharacter.h"

#include "AI/EnemyManager.h"
#include "Components/BoxComponent.h"
#include "Kismet/GameplayStatics.h"

// Sets default values
AEnemyCharacter::AEnemyCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	
	// Box = CreateDefaultSubobject<UBoxComponent>(FName("Box"));
	// Box->OnComponentBeginOverlap.AddDynamic(this, &AEnemyCharacter::OnBoxBeginOverlap);
	
	HealthComp = CreateDefaultSubobject<UHealthComponent>(FName("Health"));

	HurtboxComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("Hurtbox"));
	HurtboxComponent->SetupAttachment(GetRootComponent());
	HurtboxComponent->SetCollisionObjectType(ECC_Pawn);
	HurtboxComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	HurtboxComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	HurtboxComponent->SetCollisionResponseToChannel(ECC_GameTraceChannel3, ECR_Overlap);
	HurtboxComponent->SetGenerateOverlapEvents(true);
	HurtboxComponent->ComponentTags.Add(FName("Hurtbox"));
	HurtboxComponent->SetBoxExtent(FVector(32.f, 32.f, 80.f));
}

// Called when the game starts or when spawned
void AEnemyCharacter::BeginPlay()
{
	Super::BeginPlay();
	AttackManager = GetWorld()->GetSubsystem<UEnemyManager>();
	
	if (!AttackManager)
	{
		UE_LOG(LogTemp, Error, TEXT("=== COMBAT MANAGER IS NULL ==="));
		UE_LOG(LogTemp, Error, TEXT("Enemy: %s"), *GetName());
		UE_LOG(LogTemp, Error, TEXT("World: %s"), *GetWorld()->GetMapName());
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Combat Manager found successfully!"));
	}
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

bool AEnemyCharacter::CanAttack()
{
	if (AttackManager) return AttackManager->RequestAttack(this);
	return false;
}

void AEnemyCharacter::AttackFinished()
{
	if (AttackManager) AttackManager->ReleaseToken(this);
}

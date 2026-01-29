// Fill out your copyright notice in the Description page of Project Settings.


#include "Characters/EnemyCharacter.h"

#include "PaperFlipbookComponent.h"
#include "AI/EnemyAIController.h"
#include "AI/EnemyManager.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"

// Sets default values
AEnemyCharacter::AEnemyCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	
	// Box = CreateDefaultSubobject<UBoxComponent>(FName("Box"));
	// Box->OnComponentBeginOverlap.AddDynamic(this, &AEnemyCharacter::OnBoxBeginOverlap);
	
	AIController = Cast<AEnemyAIController>(Controller);	
	HealthComp = CreateDefaultSubobject<UHealthComponent>(FName("Health"));
}

// Called when the game starts or when spawned
void AEnemyCharacter::BeginPlay()
{
	Super::BeginPlay();
	AttackManager = GetWorld()->GetSubsystem<UEnemyAttackManager>();
}

// Called every frame
void AEnemyCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AEnemyCharacter::ActivateEnemy()
{
	// Reset Collisions
	SetActorEnableCollision(true);
	
	// Reset Movement
	GetCharacterMovement()->GravityScale = 1.0f;
	GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	
	// Restart Animations
	
	// Make Enemy Visible
	SetActorHiddenInGame(false);
	MaskSprite->SetVisibility(true);
	GetRootComponent()->SetVisibility(true);
	
	// AI Logic
	if (AIController) AIController->ActivateEnemyBT();
}

void AEnemyCharacter::DeactivateEnemy()
{	
	GetCharacterMovement()->GravityScale = 0.0f;
	
	// Stop Animations
	
	// Make Enemy not Visible
	SetActorHiddenInGame(true);
	MaskSprite->SetVisibility(false);
	GetRootComponent()->SetVisibility(false);
	
	// Health
	HealthComp->Reset();
	
	// Stop AI Logic
	if (AIController) AIController->DeactivateEnemyBT();
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

void AEnemyCharacter::OnDeath()
{
	// Deactivate Movement
	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->SetMovementMode(MOVE_None);
	
	// Deactivate Collisions
	SetActorEnableCollision(false);
	
	// Check if has Pending Token
	AttackManager->ReleaseToken(this);
}

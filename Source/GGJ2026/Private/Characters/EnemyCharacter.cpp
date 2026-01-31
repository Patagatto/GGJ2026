// Fill out your copyright notice in the Description page of Project Settings.


#include "Characters/EnemyCharacter.h"

#include "PaperFlipbookComponent.h"
#include "AI/EnemyAIController.h"
#include "AI/EnemyManager.h"
#include "Characters/GGJCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/BoxComponent.h" 
#include "Kismet/GameplayStatics.h"

// Sets default values
AEnemyCharacter::AEnemyCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	
	// --- Character Rotation Setup ---
	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;
			
	HealthComp = CreateDefaultSubobject<UHealthComponent>(FName("Health"));
	
	// --- Sprite Component Setup ---
	GetSprite()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetSprite()->SetCastShadow(true);

	// IMPORTANT: Capsule rotates for movement logic, but Sprite must stay fixed (Billboard)
	// to face the camera. PaperZD handles the visual direction via animation.
	GetSprite()->SetUsingAbsoluteRotation(true);
	GetSprite()->SetRelativeRotation(FRotator(0.0f, 0.0f, 0.0f));

	HurtboxComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("Hurtbox"));
	HurtboxComponent->SetupAttachment(GetSprite());
	HurtboxComponent->SetBoxExtent(FVector(32.f, 32.f, 80.f));
	HurtboxComponent->SetCollisionObjectType(ECC_GameTraceChannel4);
	HurtboxComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	HurtboxComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	HurtboxComponent->SetCollisionResponseToChannel(ECC_GameTraceChannel3, ECR_Overlap);
	HurtboxComponent->SetGenerateOverlapEvents(true);
	HurtboxComponent->ComponentTags.Add(FName("Hurtbox"));
}

// Called when the game starts or when spawned
void AEnemyCharacter::BeginPlay()
{
	Super::BeginPlay();
	
	//SetActorEnableCollision(false);
	
	AttackManager = GetWorld()->GetSubsystem<UEnemyAttackManager>();
	AIController = Cast<AEnemyAIController>(Controller);
	
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
	GetSprite()->SetVisibility(true);
	
	// AI Logic
	if (AIController) AIController->ActivateEnemyBT();
}

void AEnemyCharacter::DeactivateEnemy()
{	
	GetCharacterMovement()->GravityScale = 0.0f;
	
	// Stop Animations
	
	// Make Enemy not Visible
	SetActorHiddenInGame(true);
	GetSprite()->SetVisibility(false);
	
	// Health
	HealthComp->Reset();
	
	// Stop AI Logic
	if (AIController) AIController->DeactivateEnemyBT();
	SetActorLocation(SpawnLocation);
}

void AEnemyCharacter::OnBoxBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
                                        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (OtherActor && OtherActor->IsA<AGGJCharacter>())
	{
		
		UGameplayStatics::ApplyDamage(OtherActor, Damage, GetController(), this, UDamageType::StaticClass());
	}
}

float AEnemyCharacter::TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent,
	class AController* EventInstigator, AActor* DamageCauser)
{
	const float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	if (ActualDamage > 0.f && DamageCauser && DamageCauser->IsA<AGGJCharacter>())
	{
		HealthComp->ApplyDamage(ActualDamage);
		if(HealthComp->IsActorDead())
		{
			OnDeath();
			DeactivateEnemy();
		}
	}
	
	return ActualDamage;
}

bool AEnemyCharacter::CanAttack()
{
	if (AttackManager)
	{
		return AttackManager->RequestAttack(this);
	}
	
	return false;
}

void AEnemyCharacter::AttackFinished()
{
	if (AttackManager)
	{
		IsAttacking = false;
		AttackManager->ReleaseToken(this);
	}
}

void AEnemyCharacter::OnDeath()
{
	// Deactivate Movement
	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->SetMovementMode(MOVE_None);
	
	// Deactivate Collisions
	SetActorEnableCollision(false);
	
	// Check if has Pending Token
	if (AttackManager) AttackManager->ReleaseToken(this);
}

// Fill out your copyright notice in the Description page of Project Settings.


#include "Characters/EnemyCharacter.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "PaperFlipbookComponent.h"
#include "AI/EnemyAIController.h"
#include "AI/EnemyManager.h"
#include "Characters/GGJCharacter.h"
#include "Items/MaskPickup.h"
#include "Components/BoxComponent.h" 
#include "Kismet/GameplayStatics.h"

// Sets default values
AEnemyCharacter::AEnemyCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{	
	GetCapsuleComponent()->InitCapsuleSize(30.f, 85.0f);
	GetCapsuleComponent()->SetUseCCD(true);
	
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
	HurtboxComponent->SetBoxExtent(FVector(20.f, 10.f, 40.f));
	HurtboxComponent->SetCollisionObjectType(ECC_GameTraceChannel4);
	HurtboxComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	HurtboxComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	HurtboxComponent->SetCollisionResponseToChannel(ECC_GameTraceChannel3, ECR_Overlap);
	HurtboxComponent->SetGenerateOverlapEvents(true);
	HurtboxComponent->ComponentTags.Add(TEXT("Hurtbox"));
	HurtboxComponent->OnComponentBeginOverlap.AddDynamic(this, &AEnemyCharacter::OnBoxBeginOverlap);
	
	// Hitbox: Deals damage4
	HitboxComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("Hitbox"));
		
	if (HitboxComponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Constructor] HitboxComponent created: %s"), 
			*HitboxComponent->GetName());
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[Constructor] FAILED to create HitboxComponent!"));
	}
	
	// IMPORTANT: Attach to Sprite, not Root.
	// This ensures the hitbox moves correctly when the sprite is flipped (Scale X = -1).
	HitboxComponent->SetupAttachment(GetSprite()); 
	HitboxComponent->SetBoxExtent(FVector(30.f, 30.f, 30.f));
	// Set the hitbox to use our custom "PlayerHitbox" channel
	HitboxComponent->SetCollisionObjectType(ECC_GameTraceChannel5);
	// It should ignore everything by default...
	HitboxComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	HitboxComponent->SetCollisionResponseToChannel(ECC_GameTraceChannel6, ECR_Overlap);
	HitboxComponent->SetGenerateOverlapEvents(true);
	HitboxComponent->ComponentTags.Add(TEXT("Hitbox"));
	HitboxComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision); // Disabled by default! Enabled by Animation.
	
	HitboxComponent->OnComponentBeginOverlap.AddDynamic(this, &AEnemyCharacter::OnBoxBeginOverlap);
	bHasHitPlayer = false;
}

// Called when the game starts or when spawned
void AEnemyCharacter::BeginPlay()
{
	Super::BeginPlay();
		
	if (HitboxComponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("[BeginPlay] HitboxComponent valid: %s"), 
			*HitboxComponent->GetName());
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[BeginPlay] HitboxComponent is nullptr!"));
	}
	
	AttackManager = GetWorld()->GetSubsystem<UEnemyAttackManager>();
	AIController = Cast<AEnemyAIController>(Controller);
	
	const FRotator CameraRotation = GetCameraRotation();
	const float InitialYaw = CameraRotation.Yaw + AnimDirection;
	LastFacingDirection = FRotator(0.0f, InitialYaw, 0.0f).Vector();
}

// Called every frame
void AEnemyCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	UpdateAnimationDirection();
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
	// FIX: Ignore the Player's Hitbox (Weapon) to prevent taking contact damage when attacking the enemy.
	// ECC_GameTraceChannel3 corresponds to the PlayerHitbox channel.
	if (OtherComp && OtherComp->GetCollisionObjectType() == ECC_GameTraceChannel3) return;

	if (OtherActor && OtherActor->IsA<AGGJCharacter>())
	{
		
		UE_LOG(LogTemp, Warning, TEXT("Enemy touched Player! Dealing %.1f Damage"), Damage);

		UGameplayStatics::ApplyDamage(OtherActor, Damage, GetController(), this, UDamageType::StaticClass());
		
		// If the component that touched the player was the Weapon (Hitbox), mark as hit.
		if (OverlappedComp == HitboxComponent)
		{
			bHasHitPlayer = true;
		}
	}
}

float AEnemyCharacter::TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent,
	class AController* EventInstigator, AActor* DamageCauser)
{
	const float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	const bool bValidSource = DamageCauser && (DamageCauser->IsA<AGGJCharacter>() || DamageCauser->IsA<AMaskPickup>());

	if (ActualDamage > 0.f && bValidSource)
	{
		HealthComp->ApplyDamage(ActualDamage);
		OnEnemyHit();
		
		if(HealthComp->IsActorDead())
		{
			OnDeath();
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
		HitboxComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}

void AEnemyCharacter::OnDeath()
{
	// Trigger Blueprint Event (Sound, VFX, Animation)
	OnEnemyDied();

	// Deactivate Movement
	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->SetMovementMode(MOVE_None);
	
	// Deactivate Collisions
	SetActorEnableCollision(false);
	
	// Check if has Pending Token
	if (AttackManager) AttackManager->ReleaseToken(this);
}

void AEnemyCharacter::ActivateMeleeHitbox(FName SocketName, FVector Extent)
{
	// Safety Check: If socket doesn't exist on current sprite, warn and abort.
	if (!GetSprite()->DoesSocketExist(SocketName))
	{
		return; // Avoid attaching to root by mistake
	}

	if (HitboxComponent)
	{
		HitboxComponent->AttachToComponent(GetSprite(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, SocketName);
		HitboxComponent->SetBoxExtent(Extent);
		HitboxComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		
		// Reset hit tracker for this new swing
		bHasHitPlayer = false;
	}	
}

void AEnemyCharacter::DeactivateMeleeHitbox()
{
	HitboxComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	
	// Trigger the Blueprint event with the result of the attack
	OnAttackCompleted(bHasHitPlayer);
}

void AEnemyCharacter::UpdateAnimationDirection()
{
	const FVector Velocity = GetVelocity();

	// Update LastFacingDirection from Velocity if moving significantly.
	// If not moving, LastFacingDirection retains the last Input (updated in ApplyMovementInput).
	if (Velocity.SizeSquared2D() > 1.0f)
	{
		LastFacingDirection = Velocity.GetSafeNormal2D();
	}
	
	const FRotator CameraRotation = GetCameraRotation();
	const FRotator TargetRotation = LastFacingDirection.ToOrientationRotator();

	// Calculate the angle difference between the camera direction and velocity direction
	// This allows the AnimBP to select the correct directional animation (Front, Back, Side)
	const float DeltaYaw = FRotator::NormalizeAxis(TargetRotation.Yaw - CameraRotation.Yaw);

	AnimDirection = DeltaYaw;

	// Flip sprite based on movement direction relative to the camera.
	// DeltaYaw is approx -90 for left (flip), +90 for right (normal).
	if (DeltaYaw < -5.0f && DeltaYaw > -175.0f)
	{
		GetSprite()->SetRelativeScale3D(FVector(-1.0f, 1.0f, 1.0f));
	}
	else
	{
		GetSprite()->SetRelativeScale3D(FVector(1.0f, 1.0f, 1.0f));
	}
}

FRotator AEnemyCharacter::GetCameraRotation() const
{
	if (APlayerController* PC = Cast<APlayerController>(UGameplayStatics::GetPlayerController(GetWorld(), 0)))
	{
		return PC->PlayerCameraManager ? PC->PlayerCameraManager->GetCameraRotation() : FRotator(-45.f, -90.f, 0.f);
	}
	// Fallback if no controller yet
	return FRotator(-45.f, -90.f, 0.f);
}

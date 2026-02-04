// Fill out your copyright notice in the Description page of Project Settings.


#include "Items/MaskPickup.h"
#include "PaperFlipbookComponent.h"
#include "Components/BoxComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Characters/EnemyCharacter.h"
#include "DrawDebugHelpers.h"

AMaskPickup::AMaskPickup()
{
	PrimaryActorTick.bCanEverTick = true;

	// Root component
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));

	// Sprite for the mask on the ground
	Sprite = CreateDefaultSubobject<UPaperFlipbookComponent>(TEXT("Sprite"));
	Sprite->SetupAttachment(RootComponent);
	Sprite->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// Volume to detect player interaction
	InteractionVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionVolume"));
	InteractionVolume->SetupAttachment(RootComponent);
	InteractionVolume->SetCollisionProfileName(TEXT("Trigger"));
	InteractionVolume->SetBoxExtent(FVector(50.f, 50.f, 50.f));
	InteractionVolume->OnComponentBeginOverlap.AddDynamic(this, &AMaskPickup::OnOverlapBegin);

	// Damage Volume (Dedicated Hitbox)
	DamageVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("DamageVolume"));
	DamageVolume->SetupAttachment(RootComponent);
	DamageVolume->SetBoxExtent(FVector(80.f, 80.f, 80.f));
	
	// Use PlayerHitbox channel to ensure detection by enemies
	DamageVolume->SetCollisionObjectType(ECC_GameTraceChannel3);
	DamageVolume->SetGenerateOverlapEvents(true);

	DamageVolume->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	DamageVolume->SetCollisionResponseToAllChannels(ECR_Ignore);
	DamageVolume->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	DamageVolume->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);
	// Ensure we hit Enemy Hurtboxes (often on Channel 4)
	DamageVolume->SetCollisionResponseToChannel(ECC_GameTraceChannel4, ECR_Overlap);
	DamageVolume->OnComponentBeginOverlap.AddDynamic(this, &AMaskPickup::OnDamageOverlapBegin);

	// Projectile Movement
	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->UpdatedComponent = RootComponent;
	ProjectileMovement->InitialSpeed = 0.0f;
	ProjectileMovement->MaxSpeed = 2000.0f;
	ProjectileMovement->bRotationFollowsVelocity = false;
	ProjectileMovement->bShouldBounce = false;
	ProjectileMovement->ProjectileGravityScale = 0.0f;
}

void AMaskPickup::BeginPlay()
{
	Super::BeginPlay();

	// Enforce collision settings to override potential Blueprint changes
	if (DamageVolume)
	{
		DamageVolume->SetCollisionObjectType(ECC_GameTraceChannel3); // PlayerHitbox
		DamageVolume->SetCollisionResponseToAllChannels(ECR_Ignore);
		DamageVolume->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
		DamageVolume->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);
		DamageVolume->SetCollisionResponseToChannel(ECC_GameTraceChannel4, ECR_Overlap); // EnemyHurtbox
		DamageVolume->SetGenerateOverlapEvents(true);
	}
}

void AMaskPickup::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bIsFlying)
	{
		Sprite->AddLocalRotation(FRotator(RotationSpeed * DeltaTime, 0.0f, 0.0f));

		// Check if off-screen to destroy
		CheckOffScreen();
	}
}

void AMaskPickup::InitializeThrow(FVector Direction, AActor* InShooter)
{
	Shooter = InShooter;
	bIsFlying = true;
	
	// Activate movement
	ProjectileMovement->Velocity = Direction * 1500.0f; // Throw Speed
	
	// Tilt sprite to lie flat (flying disc effect)
	Sprite->SetRelativeRotation(FRotator(-90.0f, 0.0f, 0.0f));

	// Enable the dedicated Damage Volume
	DamageVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	DamageVolume->UpdateOverlaps();
}

void AMaskPickup::OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!bIsFlying) return;
	if (OtherActor == this || OtherActor == Shooter) return;

}

void AMaskPickup::OnDamageOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!bIsFlying) return;
	if (OtherActor == this || OtherActor == Shooter) return;

	// Deal damage to enemies
	if (OtherActor->IsA<AEnemyCharacter>())
	{
		if (ThrowDamage <= 0.0f)
		{
			UE_LOG(LogTemp, Error, TEXT("Mask Hit Enemy but ThrowDamage is 0! Check Blueprint defaults."));
		}

		UGameplayStatics::ApplyDamage(OtherActor, ThrowDamage, Shooter ? Shooter->GetInstigatorController() : nullptr, this, UDamageType::StaticClass());
	}
}

void AMaskPickup::CheckOffScreen()
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (!PC) return;

	FVector2D ScreenLoc;
	int32 SizeX, SizeY;
	PC->GetViewportSize(SizeX, SizeY);

	// Project world location to screen
	if (PC->ProjectWorldLocationToScreen(GetActorLocation(), ScreenLoc))
	{
		// Add a margin so it fully leaves the screen before disappearing
		float Margin = 100.0f;
		if (ScreenLoc.X < -Margin || ScreenLoc.X > SizeX + Margin ||
			ScreenLoc.Y < -Margin || ScreenLoc.Y > SizeY + Margin)
		{
			Destroy();
		}
	}
}

void AMaskPickup::UpdateVisuals(UPaperFlipbook* RedBook, UPaperFlipbook* GreenBook, UPaperFlipbook* BlueBook)
{
	UPaperFlipbook* TargetBook = nullptr;
	switch (MaskType)
	{
	case EEnemyType::RedRabbit:
		TargetBook = RedBook;
		break;
	case EEnemyType::GreenBird:
		TargetBook = GreenBook;
		break;
	case EEnemyType::BlueCat:
		TargetBook = BlueBook;
		break;
	default: break;
	}

	if (TargetBook)
	{
		Sprite->SetFlipbook(TargetBook);
	}
}
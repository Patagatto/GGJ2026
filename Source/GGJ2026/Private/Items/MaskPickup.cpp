// Fill out your copyright notice in the Description page of Project Settings.


#include "Items/MaskPickup.h"
#include "PaperFlipbookComponent.h"
#include "Components/BoxComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Characters/EnemyCharacter.h"

AMaskPickup::AMaskPickup()
{
	PrimaryActorTick.bCanEverTick = true; // Enable tick for rotation and off-screen check

	// Root component
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));

	// Sprite for the mask on the ground
	Sprite = CreateDefaultSubobject<UPaperFlipbookComponent>(TEXT("Sprite"));
	Sprite->SetupAttachment(RootComponent);
	Sprite->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// Volume to detect player interaction
	InteractionVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionVolume"));
	InteractionVolume->SetupAttachment(RootComponent);
	InteractionVolume->SetCollisionProfileName(TEXT("Trigger")); // "Trigger" profile overlaps with Pawns
	InteractionVolume->SetBoxExtent(FVector(50.f, 50.f, 50.f));
	InteractionVolume->OnComponentBeginOverlap.AddDynamic(this, &AMaskPickup::OnOverlapBegin);

	// Projectile Movement
	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->UpdatedComponent = RootComponent;
	ProjectileMovement->InitialSpeed = 0.0f;
	ProjectileMovement->MaxSpeed = 2000.0f;
	ProjectileMovement->bRotationFollowsVelocity = false;
	ProjectileMovement->bShouldBounce = false;
	ProjectileMovement->ProjectileGravityScale = 0.0f; // Fly straight
}

void AMaskPickup::BeginPlay()
{
	Super::BeginPlay();
}

void AMaskPickup::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bIsFlying)
	{
		// Rotate the sprite visually
		Sprite->AddLocalRotation(FRotator(0.0f, 0.0f, RotationSpeed * DeltaTime));

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
	
	// Enable collision with enemies (ensure Trigger profile overlaps Pawn/Character)
	InteractionVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
}

void AMaskPickup::OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!bIsFlying) return;
	if (OtherActor == this || OtherActor == Shooter) return;

	// Deal damage to enemies
	if (OtherActor->IsA<AEnemyCharacter>())
	{
		UGameplayStatics::ApplyDamage(OtherActor, ThrowDamage, Shooter ? Shooter->GetInstigatorController() : nullptr, this, UDamageType::StaticClass());
		// We do NOT destroy the mask here, it passes through
	}
	
	// Note: Player catching logic is handled in the Player's Interact() function, 
	// not here, to avoid race conditions or accidental pickups.
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
	case EMaskType::RedRabbit:
		TargetBook = RedBook;
		break;
	case EMaskType::GreenBird:
		TargetBook = GreenBook;
		break;
	case EMaskType::BlueCat:
		TargetBook = BlueBook;
		break;
	default: break;
	}

	if (TargetBook)
	{
		Sprite->SetFlipbook(TargetBook);
	}
}
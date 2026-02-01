// Fill out your copyright notice in the Description page of Project Settings.


#include "Camera/SharedCamera.h"
#include "Camera/CameraComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"

ASharedCamera::ASharedCamera()
{
	PrimaryActorTick.bCanEverTick = true;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	CameraComponent->SetupAttachment(RootComponent);

	// Default Settings
	FixedCameraRotation = FRotator(-40.0f, -90.0f, 0.0f);
	BaseDistance = 12000.0f;
	CameraLagSpeed = 5.0f;
	MinZoomDistance = 500.0f;
	MaxZoomDistance = 15000.0f;
	ZoomFactor = 0.75f;
	ScreenEdgePadding = 50.0f;
	CameraComponent->FieldOfView = 15.0f;
}

void ASharedCamera::BeginPlay()
{
	Super::BeginPlay();

	// Set the initial rotation
	SetActorRotation(FixedCameraRotation);

	// Validate configuration
	if (MaxZoomDistance < BaseDistance)
	{
		MaxZoomDistance = BaseDistance + 5000.0f;
	}

	// Snap camera to initial position immediately to avoid drift on start
	UpdateCameraPosition(1000.0f);
}

void ASharedCamera::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (GEngine)
	{
		//GEngine->AddOnScreenDebugMessage(50, 0.0f, FColor::Cyan, FString::Printf(TEXT("CAMERA DIST: %.0f | BASE: %.0f | MAX: %.0f"), FVector::Dist(GetActorLocation(), GetActorLocation() + GetActorForwardVector() * 100), BaseDistance, MaxZoomDistance));
	}

	UpdateCameraPosition(DeltaTime);
	ClampPlayersToScreen();
}

void ASharedCamera::UpdateCameraPosition(float DeltaTime)
{
	TArray<AActor*> Players;
	GetActivePlayerPawns(Players);

	if (Players.Num() == 0) return;

	FVector SumLocation = FVector::ZeroVector;
	int32 Count = 0;
	float MaxSpreadSq = 0.0f;

	// Calculate Centroid and Spread
	for (AActor* Player : Players)
	{
		if (Player)
		{
			SumLocation += Player->GetActorLocation();
			
			// Check distance against all other players to find max spread
			for (AActor* OtherPlayer : Players)
			{
				if (Player != OtherPlayer && OtherPlayer)
				{
					float DistSq = FVector::DistSquared(Player->GetActorLocation(), OtherPlayer->GetActorLocation());
					if (DistSq > MaxSpreadSq) MaxSpreadSq = DistSq;
				}
			}
			Count++;
		}
	}

	if (Count > 0)
	{
		FVector AverageLocation = SumLocation / Count;

		// Calculate target zoom distance based on player spread
		float CurrentSpread = FMath::Sqrt(MaxSpreadSq);
		float TargetDistance = BaseDistance + (CurrentSpread * ZoomFactor);
		
		TargetDistance = FMath::Clamp(TargetDistance, MinZoomDistance, MaxZoomDistance);
		
		// Determine target position using fixed rotation and calculated distance
		FVector CameraOffset = -FixedCameraRotation.Vector() * TargetDistance;
		FVector TargetLocation = AverageLocation + CameraOffset;

		// Smoothly interpolate to target
		FVector NewLocation = FMath::VInterpTo(GetActorLocation(), TargetLocation, DeltaTime, CameraLagSpeed);
		SetActorLocation(NewLocation);

		// Ensure rotation stays fixed
		SetActorRotation(FixedCameraRotation);
	}
}

void ASharedCamera::ClampPlayersToScreen()
{
	// Use Player 0 controller for viewport projection
	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (!PC) return;

	int32 SizeX, SizeY;
	PC->GetViewportSize(SizeX, SizeY);

	if (SizeX <= 0 || SizeY <= 0) return;

	TArray<AActor*> Players;
	GetActivePlayerPawns(Players);

	for (AActor* Player : Players)
	{
		if (!Player) continue;

		FVector PlayerLoc = Player->GetActorLocation();
		FVector2D ScreenLoc;

		// Project world location to screen space
		if (PC->ProjectWorldLocationToScreen(PlayerLoc, ScreenLoc))
		{
			bool bClamped = false;

			// Clamp screen coordinates with padding
			if (ScreenLoc.X < ScreenEdgePadding) { ScreenLoc.X = ScreenEdgePadding; bClamped = true; }
			else if (ScreenLoc.X > SizeX - ScreenEdgePadding) { ScreenLoc.X = SizeX - ScreenEdgePadding; bClamped = true; }

			if (ScreenLoc.Y < ScreenEdgePadding) { ScreenLoc.Y = ScreenEdgePadding; bClamped = true; }
			else if (ScreenLoc.Y > SizeY - ScreenEdgePadding) { ScreenLoc.Y = SizeY - ScreenEdgePadding; bClamped = true; }

			// If clamped, deproject back to world space to find the new valid position
			if (bClamped)
			{
				FVector WorldPos, WorldDir;
				if (PC->DeprojectScreenPositionToWorld(ScreenLoc.X, ScreenLoc.Y, WorldPos, WorldDir))
				{
					// Ray-Plane Intersection:
					// Find where the deprojected ray intersects the player's current Z plane.
					// t = (TargetZ - RayOriginZ) / RayDirectionZ
					if (FMath::Abs(WorldDir.Z) > KINDA_SMALL_NUMBER)
					{
						float t = (PlayerLoc.Z - WorldPos.Z) / WorldDir.Z;
						FVector NewLoc = WorldPos + (WorldDir * t);
						
						// Update location with sweep to respect collision
						Player->SetActorLocation(NewLoc, true);
					}
				}
			}
		}
	}
}

void ASharedCamera::GetActivePlayerPawns(TArray<AActor*>& OutPlayers) const
{
	OutPlayers.Reset();
	// Efficiently iterate local player controllers to find pawns
	for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
	{
		APlayerController* PC = Iterator->Get();
		if (PC && PC->GetPawn())
		{
			OutPlayers.Add(PC->GetPawn());
		}
	}
}
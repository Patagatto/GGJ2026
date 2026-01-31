// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SharedCamera.generated.h"

UCLASS()
class GGJ2026_API ASharedCamera : public AActor
{
	GENERATED_BODY()
	
public:	
	ASharedCamera();

protected:
	virtual void Tick(float DeltaTime) override;
	virtual void BeginPlay() override;

	/** The Camera component that renders the scene */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	class UCameraComponent* CameraComponent;

	/** Fixed rotation angle for the camera. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Settings")
	FRotator FixedCameraRotation;

	/** Base distance from the centroid of players. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Settings")
	float BaseDistance;

	/** Interpolation speed for camera movement. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Settings")
	float CameraLagSpeed;

	/** Minimum distance clamp for the camera zoom. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Settings")
	float MinZoomDistance;

	/** Maximum distance clamp for the camera zoom. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Settings")
	float MaxZoomDistance;

	/** Multiplier for zoom calculation based on player spread. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Settings")
	float ZoomFactor;

	/** Screen edge padding in pixels to constrain player movement. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Settings")
	float ScreenEdgePadding;

	void UpdateCameraPosition(float DeltaTime);

	/** Constrains player positions to the visible viewport bounds. */
	void ClampPlayersToScreen();

	/** Helper to get all active player pawns efficiently without iterating the whole world. */
	void GetActivePlayerPawns(TArray<AActor*>& OutPlayers) const;
};
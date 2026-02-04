// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MaskPickup.generated.h"

class UPaperFlipbookComponent;
class UBoxComponent;
class UProjectileMovementComponent;

/** Enum to define the different types of masks available. */
UENUM(BlueprintType)
enum class EEnemyType : uint8
{
	None		UMETA(DisplayName = "None"),
	RedRabbit	UMETA(DisplayName = "Red Rabbit"),
	GreenBird	UMETA(DisplayName = "Green Bird"),
	BlueCat		UMETA(DisplayName = "Blue Cat")
};

UCLASS()
class GGJ2026_API AMaskPickup : public AActor
{
	GENERATED_BODY()
	
public:	
	AMaskPickup();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UPaperFlipbookComponent* Sprite;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UBoxComponent* InteractionVolume;
	
	/** Larger hitbox for dealing damage when thrown. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UBoxComponent* DamageVolume;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UProjectileMovementComponent* ProjectileMovement;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mask|Combat")
	float ThrowDamage = 30;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mask|Combat")
	float RotationSpeed = 720.0f;
	
	void InitializeThrow(FVector Direction, AActor* InShooter);
	
	bool IsFlying() const { return bIsFlying; }
	
	void UpdateVisuals(class UPaperFlipbook* RedBook, UPaperFlipbook* GreenBook,UPaperFlipbook* BlueBook);

	/** Mask type, editable per instance. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mask")
	EEnemyType MaskType = EEnemyType::RedRabbit;
	
protected:
	virtual void Tick(float DeltaTime) override;
	virtual void BeginPlay() override;
	
	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent,int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnDamageOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

private:
	bool bIsFlying = false;
	UPROPERTY()
	AActor* Shooter;
	
	void CheckOffScreen();
};
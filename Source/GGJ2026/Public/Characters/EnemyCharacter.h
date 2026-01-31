// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AI/EnemyAIController.h"
#include "AI/EnemyManager.h"
#include "PaperZDCharacter.h"
#include "Components/BoxComponent.h"
#include "Components/HealthComponent.h"
#include "EnemyCharacter.generated.h"

class UBoxComponent;

UENUM(BlueprintType)
enum class EEnemyType : uint8
{
	Maskless	UMETA(DisplayName = "Maskless"),
	Rabbit		UMETA(DisplayName = "Rabbit"),
	Bird		UMETA(DisplayName = "Bird"),
	Cat			UMETA(DisplayName = "Cat"),
};

UCLASS(Abstract)
class GGJ2026_API AEnemyCharacter : public APaperZDCharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AEnemyCharacter(const FObjectInitializer& ObjectInitializer);

protected:	
	UPROPERTY(EditAnywhere)
	float Damage;
	
	/** Tracks if the current attack has hit the player. Resets on new attack. */
	bool bHasHitPlayer = false;
	
	// UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	// UBoxComponent* Box;
		
	UPROPERTY()
	UEnemyAttackManager* AttackManager;
			
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UHealthComponent* HealthComp;
	
	UPROPERTY()
	AEnemyAIController* AIController;
	
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	/** Component that detects incoming damage (The Body) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Combat, meta = (AllowPrivateAccess = "true"))
	UBoxComponent* HurtboxComponent;

	/** Component that deals damage (The Weapon) - Enabled only during attacks */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Combat, meta = (AllowPrivateAccess = "true"))
	UBoxComponent* HitboxComponent;
	
	/** The last valid direction the character was moving or inputting towards (World Space) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GGJ|Debug", meta = (DisplayPriority = "0"))
	FVector LastFacingDirection;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GGJ|Debug", meta = (DisplayPriority = "0"))
	float AnimDirection;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EEnemyType Type;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool IsAttacking = false;
		
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FVector SpawnLocation;
	
	UFUNCTION(BlueprintImplementableEvent, Category = "Combat")
	void OnEnemyDied();
	
	/** Event called when the attack hitbox is deactivated. bHitPlayer is true if damage was dealt. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Combat")
	void OnAttackCompleted(bool bHitPlayer);

	// Called every frame
	virtual void Tick(float DeltaTime) override;
	
	void ActivateEnemy();
	
	UFUNCTION(BlueprintCallable)
	void DeactivateEnemy();
		
	UFUNCTION()
	void OnBoxBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;
	
	UFUNCTION(BlueprintCallable)
	bool CanAttack();
	
	// To call at the end of the Attack animation
	UFUNCTION(BlueprintCallable)
	void AttackFinished();
		
	// To call the moment it dies
	UFUNCTION(BlueprintCallable)
	void OnDeath();
	
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void ActivateMeleeHitbox(FName SocketName, FVector Extent = FVector(30.f, 30.f, 30.f));
	
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void DeactivateMeleeHitbox();
};

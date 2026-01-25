// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "PaperZDCharacter.h"
#include "InputActionValue.h"
#include "GGJCharacter.generated.h"

class UCameraComponent;
class USpringArmComponent;
class UInputMappingContext;
class UInputAction;
class UBoxComponent;
class UPaperFlipbookComponent;
class UPaperFlipbook;

/** 
 * Defines the current high-level action state of the character.
 * Mutually exclusive: You cannot be Attacking and Rolling at the same time.
 */
UENUM(BlueprintType)
enum class ECharacterActionState : uint8
{
	None		UMETA(DisplayName = "None"),
	Attacking	UMETA(DisplayName = "Attacking"),
	Rolling		UMETA(DisplayName = "Rolling"),
	Hurt		UMETA(DisplayName = "Hurt"),
	Dead		UMETA(DisplayName = "Dead")
};

/**
 * 
 */
UCLASS(Abstract)
class GGJ2026_API AGGJCharacter : public APaperZDCharacter
{
	GENERATED_BODY()
	
	virtual void Tick(float DeltaSeconds) override;

public:
	AGGJCharacter(const FObjectInitializer& ObjectInitializer);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FollowCamera;
	
	/** Component that detects incoming damage (The Body) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Combat, meta = (AllowPrivateAccess = "true"))
	UBoxComponent* HurtboxComponent;

	/** Sprite component for the equipped weapon. Attached to the main Sprite via Socket. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Combat, meta = (AllowPrivateAccess = "true"))
	UPaperFlipbookComponent* WeaponSprite;

	/** Component that deals damage (The Weapon) - Enabled only during attacks */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Combat, meta = (AllowPrivateAccess = "true"))
	UBoxComponent* HitboxComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Animation)
	float AnimDirection;
	
	/** Speed of the character (XY plane magnitude) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Animation)
	float Speed;

	/** True if the character is moving faster than a small threshold */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Animation)
	bool bIsMoving = false;
	
	/** Jump state check booleans for Anim Blueprint*/
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Animation)
	bool bIsJumping = false;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Animation)
	bool bStartJumping = false;
	
	

	/** Vertical Velocity (Z) for Jump/Fall animation states */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Animation)
	float VerticalVelocity;

	// --- Combat & Actions State ---

	/** Current Action State (None, Attacking, Rolling, Dead, etc.) */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = Animation)
	ECharacterActionState ActionState = ECharacterActionState::None;

	/** Current index for attack combos (e.g. 0=First Swing, 1=Second Swing) */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = Animation)
	int32 AttackComboIndex = 0;

	/** Time window (in seconds) after an attack to input the next combo command before it resets */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Combat)
	float ComboWindowTime = 0.8f;

	/** Max number of attacks in the combo chain (e.g. 3 for a 3-hit combo) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Combat)
	int32 MaxComboCount = 3;

	/** Delay in seconds before the jump force is applied. Useful for anticipation animations. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Jump")
	float JumpDelayTime = 0.05f;

	FTimerHandle ComboTimerHandle;

protected:
	virtual void BeginPlay() override;
	virtual void Landed(const FHitResult& Hit) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	
	void UpdateAnimationDirection();
	
	/** Handler for the Input Action (Internal C++ binding) */
	void Move(const FInputActionValue& Value);

	/** Timer handle for the jump delay */
	FTimerHandle JumpTimerHandle;

	/** Flag to track if the jump button was released during the delay */
	bool bJumpStopPending = false;

	/** Starts the jump sequence (starts timer or jumps immediately) */
	void StartJumpSequence();

	/** Handles the release of the jump button (cancels variable height or marks pending stop) */
	void StopJumpSequence();

	/** Executes the actual jump */
	void PerformJump();

	/** Internal function to reset combo when timer expires */
	void ResetCombo();

	/** Called when the Hitbox overlaps something */
	UFUNCTION()
	void OnHitboxOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

public:
	/** 
	 * Applies movement logic relative to the camera. 
	 * Exposed to Blueprint so designers can drive movement from other sources (e.g. UI, AI, Custom Scripts).
	 */
	UFUNCTION(BlueprintCallable, Category = "Movement")
	void ApplyMovementInput(FVector2D MovementVector);

	/** Equips a weapon flipbook to a specific socket on the character sprite */
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void EquipWeapon(UPaperFlipbook* WeaponFlipbook, FName SocketName = TEXT("HandSocket"));

	/** Removes the current weapon */
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void UnequipWeapon();

	/** Call this when the player presses the Attack button. Handles Combo logic automatically. */
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void PerformAttack();

	/** Call this via AnimNotify (PaperZD) when the attack animation ends or reaches a transition point. */
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void OnAttackFinished();

public:
	/** Default Input Mapping Context */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputMappingContext* DefaultMappingContext;

	/** Jump Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* JumpAction;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* MoveAction;
};

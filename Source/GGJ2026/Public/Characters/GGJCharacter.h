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
	Charging	UMETA(DisplayName = "Charging"),
	Dead		UMETA(DisplayName = "Dead")
};

/**
 * 
 */
UCLASS(Abstract, meta = (PrioritizeCategories = "GGJ"))
class GGJ2026_API AGGJCharacter : public APaperZDCharacter
{
	GENERATED_BODY()
	
	virtual void Tick(float DeltaSeconds) override;

public:
	AGGJCharacter(const FObjectInitializer& ObjectInitializer);

	// ========================================================================
	// COMPONENTS
	// ========================================================================

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FollowCamera;
	
	/** Component that detects incoming damage (The Body) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Combat, meta = (AllowPrivateAccess = "true"))
	UBoxComponent* HurtboxComponent;

	/** Component that deals damage (The Weapon) - Enabled only during attacks */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Combat, meta = (AllowPrivateAccess = "true"))
	UBoxComponent* HitboxComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Combat, meta = (AllowPrivateAccess = "true"))
	UPaperFlipbookComponent* MaskSprite;

	// ========================================================================
	// INPUT ASSETS
	// ========================================================================

	/** Default Input Mapping Context */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GGJ|Input", meta = (AllowPrivateAccess = "true", DisplayPriority = "0"))
	UInputMappingContext* DefaultMappingContext;

	/** Jump Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GGJ|Input", meta = (AllowPrivateAccess = "true", DisplayPriority = "0"))
	UInputAction* JumpAction;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GGJ|Input", meta = (AllowPrivateAccess = "true", DisplayPriority = "0"))
	UInputAction* MoveAction;

	/** Attack Input Action (Tap to Attack, Hold to Charge) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GGJ|Input", meta = (AllowPrivateAccess = "true", DisplayPriority = "0"))
	UInputAction* AttackAction;

	/** Roll Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GGJ|Input", meta = (AllowPrivateAccess = "true", DisplayPriority = "0"))
	UInputAction* RollAction;

	// ========================================================================
	// DESIGNER CONFIGURATION (EditAnywhere)
	// ========================================================================

	// --- Stats ---

	/** Max Health of the character */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GGJ|Stats", meta = (DisplayPriority = "0"))
	float MaxHealth = 100.0f;

	/** Duration of invincibility after taking damage (seconds) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GGJ|Stats", meta = (DisplayPriority = "0"))
	float InvincibilityDuration = 1.0f;

	/** Duration of stun (inability to move) after taking damage (seconds) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GGJ|Stats", meta = (DisplayPriority = "0"))
	float HitStunDuration = 0.4f;

	/** Strength of the knockback when hit */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GGJ|Stats", meta = (DisplayPriority = "0"))
	float KnockbackStrength = 600.0f;

	// --- Combat (General) ---

	/** Time window (in seconds) after an attack to input the next combo command before it resets */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GGJ|Combat", meta = (DisplayPriority = "0"))
	float ComboWindowTime = 0.8f;

	/** Max number of attacks in the combo chain (e.g. 3 for a 3-hit combo) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GGJ|Combat", meta = (DisplayPriority = "0"))
	int32 MaxComboCount = 3;

	/** Damage values for each step of the combo. Index 0 = Hit 1, Index 1 = Hit 2, etc. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GGJ|Combat", meta = (DisplayPriority = "0"))
	TArray<float> ComboDamageValues;

	// --- Combat (Lunge / Aim Assist) ---

	/** Max distance to search for an enemy to lunge towards */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GGJ|Combat|Lunge", meta = (DisplayPriority = "0"))
	float LungeRange = 400.0f;

	/** Max angle (in degrees) to consider an enemy valid. 45 = 90 degree cone in front. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GGJ|Combat|Lunge", meta = (DisplayPriority = "0"))
	float LungeHalfAngle = 60.0f;

	/** Distance from target to stop lunging (avoids clipping into enemy) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GGJ|Combat|Lunge", meta = (DisplayPriority = "0"))
	float LungeStopDistance = 60.0f;

	// --- Combat (Charge Attack) ---

	/** Max time (seconds) to reach full charge damage */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GGJ|Combat|Charge", meta = (DisplayPriority = "0"))
	float MaxChargeTime = 1.0f;

	/** Damage multiplier when fully charged */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GGJ|Combat|Charge", meta = (DisplayPriority = "0"))
	float MaxChargeDamageMultiplier = 2.0f;

	// --- Movement ---

	/** Delay in seconds before the jump force is applied. Useful for anticipation animations. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GGJ|Movement", meta = (DisplayPriority = "0"))
	float JumpDelayTime = 0.1f;

	/** Initial burst speed of the roll. Input can still influence movement after the burst. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GGJ|Movement", meta = (DisplayPriority = "0"))
	float RollSpeed = 1200.0f;

	/** Cooldown in seconds between each roll. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GGJ|Movement", meta = (DisplayPriority = "0"))
	float RollCooldown = 1.0f;

	// ========================================================================
	// RUNTIME STATE (VisibleAnywhere - Debugging)
	// ========================================================================

	/** The last valid direction the character was moving or inputting towards (World Space) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GGJ|Debug", meta = (DisplayPriority = "0"))
	FVector LastFacingDirection;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GGJ|Debug", meta = (DisplayPriority = "0"))
	float AnimDirection;

	/** Speed of the character (XY plane magnitude) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GGJ|Debug", meta = (DisplayPriority = "0"))
	float Speed;

	/** True if the character is moving faster than a small threshold */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GGJ|Debug", meta = (DisplayPriority = "0"))
	bool bIsMoving = false;

	/** Jump state check booleans for Anim Blueprint*/
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GGJ|Debug", meta = (DisplayPriority = "0"))
	bool bIsJumping = false;

	/** Trigger for the start of the jump animation */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GGJ|Debug", meta = (DisplayPriority = "0"))
	bool bStartJumping = false;

	/** Vertical Velocity (Z) for Jump/Fall animation states */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GGJ|Debug", meta = (DisplayPriority = "0"))
	float VerticalVelocity;

	/** Current Action State (None, Attacking, Rolling, Dead, etc.) */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "GGJ|Debug", meta = (DisplayPriority = "0"))
	ECharacterActionState ActionState = ECharacterActionState::None;

	/** Current index for attack combos (e.g. 0=First Swing, 1=Second Swing) */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "GGJ|Debug", meta = (DisplayPriority = "0"))
	int32 AttackComboIndex = 0;

	/** Current Health of the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GGJ|Debug", meta = (DisplayPriority = "0"))
	float CurrentHealth;

	/** Current charge duration (for AnimBP or UI) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GGJ|Debug", meta = (DisplayPriority = "0"))
	float CurrentChargeTime = 0.0f;

protected:
	virtual void BeginPlay() override;
	virtual void Landed(const FHitResult& Hit) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	
	void UpdateAnimationDirection();
	
	// --- Internal Logic & State ---

	/** Handler for the Input Action (Internal C++ binding) */
	void Move(const FInputActionValue& Value);

	float CurrentDamageMultiplier = 1.0f;
	
	// Timers
	FTimerHandle ComboTimerHandle;
	FTimerHandle JumpTimerHandle;
	FTimerHandle InvincibilityTimerHandle;
	FTimerHandle StunTimerHandle;
	FTimerHandle RollCooldownTimerHandle;
	float DefaultBrakingDeceleration;

	/** Flag to track if the jump button was released during the delay */
	bool bJumpStopPending = false;

	/** Flag to track if a combo window was active when charging started */
	bool bPendingCombo = false;

	/** If true, the character cannot roll until the cooldown expires. */
	bool bIsRollOnCooldown = false;

	// Lunge State
	bool bIsLunging = false;
	FVector LungeTargetLocation;
	float LungeStartTime;

	/** Starts the jump sequence (starts timer or jumps immediately) */
	void StartJumpSequence();

	/** Handles the release of the jump button (cancels variable height or marks pending stop) */
	void StopJumpSequence();

	/** Executes the actual jump */
	void PerformJump();

	/** Internal function to reset combo when timer expires */
	void ResetCombo();

	/** If true, character ignores incoming damage */
	bool bIsInvincible = false;

	/** Called when the stun duration ends to return to normal state */
	void OnStunFinished();
	void DisableInvincibility();
	
	/** 
	 * Finds the best target based on Input Direction.
	 * Prioritizes enemies aligned with input and close distance.
	 */
	AActor* FindBestTarget(FVector InputDirection);
	
	/**
	 * Launches the character towards the target.
	 * Calculates velocity to stop exactly near the target.
	 */
	void PerformLunge(AActor* Target);

	// Input Handlers for Charging
	void StartCharging();
	void UpdateCharging(const FInputActionValue& Value);
	void FinishCharging();
	
	/** Executes the roll logic (State change, Physics, Invincibility) */
	void PerformRoll();

	/** Resets the roll cooldown flag. */
	void ResetRollCooldown();

	/** Called when the Hitbox overlaps something */
	UFUNCTION()
	void OnHitboxOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	/** Event called when this actor takes damage */
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;

public:
	/** 
	 * Applies movement logic relative to the camera. 
	 * Exposed to Blueprint so designers can drive movement from other sources (e.g. UI, AI, Custom Scripts).
	 */
	UFUNCTION(BlueprintCallable, Category = "Movement")
	void ApplyMovementInput(FVector2D MovementVector, bool IgnoreState);

	/** Call this when the player presses the Attack button. Handles Combo logic automatically. */
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void PerformAttack();

	/** Call this via AnimNotify (PaperZD) when the attack animation ends or reaches a transition point. */
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void OnAttackFinished();

	/** Called via AnimNotify when roll animation ends */
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void OnRollFinished();

	/** 
	 * Activates the combat hitbox.
	 * To be called via AnimNotify at the start of the active punch frame.
	 * @param SocketName The name of the socket in the Flipbook where the hitbox should spawn (e.g., "HitSocket").
	 * @param Extent (Optional) The size of the hitbox for this specific attack.
	 */
	
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void ActivateMeleeHitbox(FName SocketName, FVector Extent = FVector(30.f, 30.f, 30.f));

	/** Deactivates the hitbox. To be called via AnimNotify at the end of the strike. */
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void DeactivateMeleeHitbox();

};

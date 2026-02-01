// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "PaperZDCharacter.h"
#include "InputActionValue.h"
#include "Items/MaskPickup.h"
#include "GGJCharacter.generated.h"

class UInputMappingContext;
class UInputAction;
class UBoxComponent;
class UPaperFlipbookComponent;

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
	ChargeMask  UMETA(DisplayName = "ChargeMask"),
	KnockedDown	UMETA(DisplayName = "KnockedDown"),
	Grounded	UMETA(DisplayName = "Grounded"),
	GettingUp	UMETA(DisplayName = "GettingUp"),
	Dead		UMETA(DisplayName = "Dead")
};

/**
 * 
 */
UCLASS(Abstract, meta = (PrioritizeCategories = "GGJ"))
class GGJ2026_API AGGJCharacter : public APaperZDCharacter
{
	GENERATED_BODY()

public:
	AGGJCharacter(const FObjectInitializer& ObjectInitializer);

	// ========================================================================
	// COMPONENTS
	// ========================================================================

	/** Component that detects incoming damage (The Body) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Combat, meta = (AllowPrivateAccess = "true"))
	UBoxComponent* HurtboxComponent;

	/** Component that deals damage (The Weapon) - Enabled only during attacks */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Combat, meta = (AllowPrivateAccess = "true"))
	UBoxComponent* HitboxComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Combat, meta = (AllowPrivateAccess = "true"))
	UPaperFlipbookComponent* MaskSprite;

	/** Sphere component to detect nearby interactable items like masks. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Interaction", meta = (AllowPrivateAccess = "true"))
	class USphereComponent* InteractionSphere;

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

	/** Interact Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GGJ|Input", meta = (AllowPrivateAccess = "true", DisplayPriority = "0"))
	UInputAction* InteractAction;

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
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GGJ|Stats", meta = (DisplayPriority = "0"))
	float InvincibilityTimeAfterKnock = 2.0f;

	/** Duration of stun (inability to move) after taking damage (seconds) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GGJ|Stats", meta = (DisplayPriority = "0"))
	float HitStunDuration = 0.4f;

	/** Strength of the knockback when hit */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GGJ|Stats", meta = (DisplayPriority = "0"))
	float KnockbackStrength = 600.0f;

	/** Number of consecutive hits the character can take before being knocked down. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GGJ|Stats", meta = (DisplayPriority = "1"))
	int32 HitsUntilKnockdown = 3;

	/** The force of the push when knocked down. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GGJ|Stats", meta = (DisplayPriority = "1"))
	float KnockdownPushStrength = 1200.0f;

	/** Time in seconds the character stays on the ground after being knocked down. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GGJ|Stats", meta = (DisplayPriority = "1"))
	float GroundedTime = 2.0f;

	/** Time in seconds without being hit before the knockdown hit counter resets. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GGJ|Stats", meta = (DisplayPriority = "1"))
	float HitCountResetTime = 2.0f;

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

	// --- Mask System ---

	/** The maximum duration a mask can be worn. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GGJ|Masks", meta = (DisplayPriority = "0"))
	float MaxMaskDuration = 30.0f;

	/** How quickly the mask's drain rate increases over time. Higher value = harder to maintain. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GGJ|Masks", meta = (DisplayPriority = "0"))
	float DrainIncreaseRate = 0.05f;

	// --- Mask System (Visuals) ---

	/** Flipbook asset for the Red Rabbit mask. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GGJ|Masks|Visuals", meta = (DisplayPriority = "0"))
	class UPaperFlipbook* RedRabbitMaskFlipbook;

	/** Flipbook asset for the Green Bird mask. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GGJ|Masks|Visuals", meta = (DisplayPriority = "0"))
	class UPaperFlipbook* GreenBirdMaskFlipbook;

	/** Flipbook asset for the Blue Cat mask. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GGJ|Masks|Visuals", meta = (DisplayPriority = "0"))
	class UPaperFlipbook* BlueCatMaskFlipbook;
	
	// --- Mask System  (Buffs) ---

	/** (Red Rabbit) Percentage of damage dealt that is returned as health. 0.1 = 10% Lifesteal. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GGJ|Masks|Buffs", meta = (DisplayName = "Lifesteal Percentage (Red)"))
	float RedRabbit_LifestealAmount = 0.1f;

	/** (Red Rabbit) Amount of time (in seconds) added to the mask duration when hitting an enemy. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GGJ|Masks|Buffs", meta = (DisplayName = "Mask Time Bonus on Hit (Red)"))
	float RedRabbit_TimeToAddOnHit = 1.0f;

	/** (Green Bird) Percentage of incoming damage to ignore. 0.25 = 25% damage reduction. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GGJ|Masks|Buffs", meta = (DisplayName = "Damage Reduction (Green)"))
	float GreenBird_DamageReductionAmount = 0.25f;

	/** (Blue Cat) The new, lower cooldown for the Roll ability. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GGJ|Masks|Buffs", meta = (DisplayName = "Roll Cooldown (Blue)"))
	float BlueCat_RollCooldown = 0.4f;

	/** (Blue Cat) The new, higher movement speed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GGJ|Masks|Buffs", meta = (DisplayName = "Movement Speed (Blue)"))
	float BlueCat_MovementSpeed = 800.0f;

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

	/** Number of hits taken in a row. Resets after recovering or being knocked down. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GGJ|Debug", meta = (DisplayPriority = "0"))
	int32 CurrentHitCount = 0;

	/** Current charge duration (for AnimBP or UI) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GGJ|Debug", meta = (DisplayPriority = "0"))
	float CurrentChargeTime = 0.0f;

	/** The type of mask currently equipped. Exposed for UI. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GGJ|Debug", meta = (DisplayPriority = "0"))
	EEnemyType CurrentMaskType = EEnemyType::None;

	/** The remaining duration of the current mask. Exposed for UI. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GGJ|Debug", meta = (DisplayPriority = "0"))
	float CurrentMaskDuration = 0.0f;

	/** The current multiplier for the mask's time drain. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GGJ|Debug", meta = (DisplayPriority = "0"))
	float DrainRateMultiplier = 1.0f;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void Landed(const FHitResult& Hit) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void PawnClientRestart() override;
	
	void UpdateAnimationDirection();
	
	// --- Damage & State Handlers ---

	/** Handles the logic when the character is knocked down (invincibility, pushback, state change). */
	void HandleKnockdown(AActor* DamageCauser);

	/** Handles the logic when the character takes a normal hit (stun, small pushback). */
	void HandleHurt(AActor* DamageCauser);

	/** Handles the death logic. */
	void HandleDeath();

	/** Interrupts any ongoing combat actions (Combos, Lunges, Charging). */
	void InterruptCombatActions();

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
	FTimerHandle MaskDurationTimerHandle;
	FTimerHandle HitCountResetTimerHandle;
	FTimerHandle GroundedTimerHandle;
	FTimerHandle InvincibilityOnGettingUpHandle;
	float DefaultBrakingDeceleration;
	float DefaultRollCooldown;
	float DefaultMaxWalkSpeed;

	/** Flag to track if the jump button was released during the delay */
	bool bJumpStopPending = false;

	/** Flag to track if a combo window was active when charging started */
	bool bPendingCombo = false;

	/** If true, the character cannot roll until the cooldown expires. */
	bool bIsRollOnCooldown = false;

	/** If true, hitting an enemy extends the current mask's duration. (Red Rabbit Buff) */
	bool bExtendsDurationOnHit = false;

	/** If true, incoming damage is reduced by a percentage. (Green Bird Buff) */
	bool bHasDamageReduction = false;

	/** If true, the character cannot be knocked down by consecutive hits. (Green Bird Buff) */
	bool bIsImmuneToKnockdown = false;

	/** If true, the character heals for a percentage of damage dealt. (Red Rabbit Buff) */
	bool bHasLifesteal = false;

	/** A reference to the closest mask the player can pick up. */
	UPROPERTY()
	AMaskPickup* OverlappingMask = nullptr;
	
	/** Flag to prevent immediate throwing when picking up a mask in the same input press. */
	bool bInputConsumed = false;

	// Lunge State
	bool bIsLunging = false;
	FVector LungeTargetLocation;
	float LungeStartTime;
	
	UPROPERTY()
	TArray<AActor*> HitActors;

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
	/** Called when the grounded duration ends to start getting up */
	void OnGroundedTimerFinished();

	/** Called by a timer to reset the consecutive hit counter. */
	void ResetHitCount();

	void DisableInvincibility();
	
	/** 
	 * Finds the best target based on Input Direction.
	 * Prioritizes enemies aligned with input and close distance.
	 */
	AActor* FindBestTarget(FVector InputDirection);
	
	/** 
	 * Helper to get the current view rotation (Shared Camera or Controller).
	 * Replaces the old dependency on FollowCamera component.
	 */
	FRotator GetCameraRotation() const;

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

	/** Main interaction function, called by input. */
	void Interact();
	
	void ChargeMask();
	
	void LaunchMask();

	/** Equips a new mask and starts its timer. */
	void EquipMask(AMaskPickup* MaskToEquip);

	/** Removes the current mask and its buffs. */
	void UnequipMask();

	/** Called by a timer to decrease mask duration and increase drain rate. */
	void UpdateMaskDuration();

	/** Adds time to the current mask's duration, called on enemy hit. */
	void ExtendMaskDuration();

	void ApplyBuff(EEnemyType MaskType);
	void RemoveBuff(EEnemyType MaskType);
	
	/** Called when the Hitbox overlaps something */
	UFUNCTION()
	void OnHitboxOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	/** Event called when this actor takes damage */
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;

	// Overlap functions for the interaction sphere
	UFUNCTION()
	void OnInteractionSphereOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnInteractionSphereOverlapEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
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
	
	/** Call this via AnimNotify (PaperZD) when the get up animation is finished. */
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void OnGetUpFinished();
	
	/** Event called when the attack sequence starts. Use this for initial swing sounds (Whoosh). */
	UFUNCTION(BlueprintImplementableEvent, Category = "Combat")
	void OnAttackStarted(bool bHasHitEnemy, bool bHasMask);
	
	UFUNCTION(BlueprintImplementableEvent, Category = "Combat")
	void OnAttackCompleted(bool bHasHitEnemy, bool bHasMask);

	/** Event called when the player catches a flying mask. Use this to play sound/VFX in Blueprint. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Interaction")
	void OnMaskCaught();
	
	UFUNCTION(BlueprintImplementableEvent, Category = "Interaction")
	void OnMaskPickUp();
	
	UFUNCTION(BlueprintImplementableEvent, Category = "Interaction")
	void OnPlayerHit(bool bIsKnockDown);
	
	/** Event called when the player's invincibility period ends. Use this to stop flashing effects. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Combat")
	void OnInvincibilityEnded();

	/** Event called when the player is knocked down (starts falling). Use this to play sound/VFX in Blueprint. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Combat")
	void OnKnockedDown();
	
	/** Event called when the player dies. Use this to play death sound/VFX/Animation in Blueprint. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Combat")
	void OnPlayerDied();

	/** Event called when the player starts charging an attack. Use for looping sound/VFX. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Combat")
	void OnChargeStarted();

	/** Event called when the player stops charging (release or interrupt). Use to stop looping sound/VFX. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Combat")
	void OnChargeEnded();
	
	UFUNCTION(BlueprintImplementableEvent, Category = "Outline")
	void OnMaskChanged(EEnemyType MaskType);
	
	UFUNCTION(BlueprintImplementableEvent, Category = "Outline")
	void OnGettingUp();

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
	
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void ActivateMask(FName SocketName);


private:
	/** Helper to reset time dilation back to normal. */
	void ResetGlobalTimeDilation();
	FTimerHandle TimeDilationTimerHandle;
};

// Fill out your copyright notice in the Description page of Project Settings.


#include "Characters/GGJCharacter.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "PaperFlipbookComponent.h"
#include "Components/BoxComponent.h"
#include "TimerManager.h"
#include "Characters/EnemyCharacter.h"
#include "Engine/OverlapResult.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/DamageType.h"


AGGJCharacter::AGGJCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#pragma region Components Setup
	// --- Capsule Component Setup ---
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(30.f, 85.0f);
	GetCapsuleComponent()->SetUseCCD(true);

	// --- Character Rotation Setup ---
	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// --- Camera Boom Setup ---
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 5000.0f; // Distance for Top-Down view
	CameraBoom->SocketOffset = FVector(0.0f, 0.0f, 75.0f); // Slight offset
	CameraBoom->SetUsingAbsoluteRotation(true); // Don't rotate arm with character
	
	// Adjusted camera angle to -45 degrees. This prevents the perspective from flattening vertical movement, ensuring consistent visual speed.
	CameraBoom->SetRelativeRotation(FRotator(-45.0f, -90.0f, 0.0f)); 
	
	// Disable inheritance to prevent camera jitter during character rotation
	CameraBoom->bInheritPitch = false;
	CameraBoom->bInheritYaw = false;
	CameraBoom->bInheritRoll = false;
	CameraBoom->bDoCollisionTest = false; // Disable collision for 2D/Top-Down to prevent zooming in
	CameraBoom->bEnableCameraLag = true;
	CameraBoom->CameraLagSpeed = 5.0f;

	// --- Follow Camera Setup ---
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->ProjectionMode = ECameraProjectionMode::Perspective;
	FollowCamera->SetFieldOfView(10.0f);
	FollowCamera->bUsePawnControlRotation = false;

#pragma endregion

#pragma region Movement Setup
	// --- Character Movement Setup ---
	GetCharacterMovement()->GravityScale = 2.0f;
	GetCharacterMovement()->AirControl = 0.80f;
	GetCharacterMovement()->JumpZVelocity = 1000.f;
	GetCharacterMovement()->GroundFriction = 3.0f;
	GetCharacterMovement()->MaxWalkSpeed = 600.0f;
	GetCharacterMovement()->MaxFlySpeed = 600.0f;

	// HD-2D / Top-Down Specifics:
	// Allow movement in all directions (XY plane), removing the default 2D side-scroller constraint
	GetCharacterMovement()->bConstrainToPlane = false;
	
	// Disable physical capsule rotation to prevent camera jitter.
	// Visual direction is handled solely by the Sprite/Flipbook animation.
	GetCharacterMovement()->bOrientRotationToMovement = false;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 0.0f, 0.0f);

	// Use flat base for floor checks (prevents sliding off ledges in 2D)
	GetCharacterMovement()->bUseFlatBaseForFloorChecks = true;
	
	// Initialize AnimDirection to 180 (Front/South) so the character spawns facing the camera
	AnimDirection = 180.0f;
#pragma endregion

#pragma region Visuals & Combat Setup
	// --- Sprite Component Setup ---
	GetSprite()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetSprite()->SetCastShadow(true);

	// IMPORTANT: Capsule rotates for movement logic, but Sprite must stay fixed (Billboard)
	// to face the camera. PaperZD handles the visual direction via animation.
	GetSprite()->SetUsingAbsoluteRotation(true);
	GetSprite()->SetRelativeRotation(FRotator(0.0f, 0.0f, 0.0f));

	// --- Combat Components Setup ---
	
	// Hurtbox: Detects damage taken
	HurtboxComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("Hurtbox"));
	HurtboxComponent->SetupAttachment(GetSprite()); // Attach to sprite so it follows visual representation
	HurtboxComponent->SetBoxExtent(FVector(20.f, 20.f, 40.f));
	HurtboxComponent->SetCollisionProfileName(TEXT("OverlapAllDynamic")); // Should be customized to only overlap Enemy Hitboxes
	HurtboxComponent->SetGenerateOverlapEvents(true);

	MaskSprite = CreateDefaultSubobject<UPaperFlipbookComponent>(TEXT("MaskSprite"));
	MaskSprite->SetupAttachment(GetSprite());
	MaskSprite->SetCollisionEnabled(ECollisionEnabled::NoCollision); // Purely visual
	
	// Hitbox: Deals damage
	HitboxComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("Hitbox"));
	// IMPORTANT: Attach to Sprite, not Root.
	// This ensures the hitbox moves correctly when the sprite is flipped (Scale X = -1).
	HitboxComponent->SetupAttachment(GetSprite()); 
	HitboxComponent->SetBoxExtent(FVector(30.f, 30.f, 30.f));
	HitboxComponent->SetCollisionProfileName(TEXT("OverlapAllDynamic")); // Should be customized to only overlap Enemy Hurtboxes
	HitboxComponent->SetGenerateOverlapEvents(true);
	HitboxComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision); // Disabled by default! Enabled by Animation.

	// Bind the overlap event
	HitboxComponent->OnComponentBeginOverlap.AddDynamic(this, &AGGJCharacter::OnHitboxOverlap);

	// Default Combo Damages (Low, Low, High)
	ComboDamageValues.Add(10.0f);
	ComboDamageValues.Add(10.0f);
	ComboDamageValues.Add(25.0f);

	// Initialize Health defaults
	MaxHealth = 100.0f;
	CurrentHealth = MaxHealth;

	// Combat Defaults
	InvincibilityDuration = 1.0f;
	HitStunDuration = 0.4f;
	KnockbackStrength = 600.0f;
#pragma endregion
}

void AGGJCharacter::BeginPlay()
{
	Super::BeginPlay();
	
	// Unreal defaults to Falling on spawn. We force Walking so the first frame 
	// doesn't play the fall animation if we are spawning on the ground.
	// If we are actually in the air, the next physics update will correct it to Falling.
	GetCharacterMovement()->SetMovementMode(MOVE_Walking);

	// Initialize Health
	CurrentHealth = MaxHealth;

	// Reset Combat States on Spawn
	ActionState = ECharacterActionState::None;
	AttackComboIndex = 0;
	CurrentDamageMultiplier = 1.0f;
	bJumpStopPending = false;
	bPendingCombo = false;

	// Add Input Mapping Context
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}
}

void AGGJCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Calculate speed and movement state for AnimBP
	Speed = GetVelocity().Size2D();
	bIsMoving = Speed > 1.0f;

	// Update jumping state (True if in air)
	bIsJumping = GetCharacterMovement()->IsFalling();
	VerticalVelocity = GetVelocity().Z;
	
	// DEBUG: Print values to screen to verify C++ is working
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Yellow, FString::Printf(TEXT("Speed: %.2f | Moving: %d | Dir: %.2f | Jumping:%d | Z: %.2f"), Speed, bIsMoving, AnimDirection, bIsJumping, VerticalVelocity));
	}

	UpdateAnimationDirection();
}

void AGGJCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// Bind Input Actions
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent)) {
		
		// Jump

		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &AGGJCharacter::StartJumpSequence);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &AGGJCharacter::StopJumpSequence);
		// Move
		
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AGGJCharacter::Move);

		// Attack (Charge Logic)
		EnhancedInputComponent->BindAction(AttackAction, ETriggerEvent::Started, this, &AGGJCharacter::StartCharging);
		EnhancedInputComponent->BindAction(AttackAction, ETriggerEvent::Triggered, this, &AGGJCharacter::UpdateCharging);
		EnhancedInputComponent->BindAction(AttackAction, ETriggerEvent::Completed, this, &AGGJCharacter::FinishCharging);
	}
}

#pragma region Movement Logic

void AGGJCharacter::UpdateAnimationDirection()
{
	// Only check 2D velocity (XY) to prevent rotation reset when falling vertically (Z).
	if (GetVelocity().SizeSquared2D() <= 1.0f) return;
	
	FRotator CameraRotation = FollowCamera->GetComponentRotation();
	FRotator VelocityRotation = GetVelocity().ToOrientationRotator();

	// Calculate the angle difference between the camera direction and velocity direction
	// This allows the AnimBP to select the correct directional animation (Front, Back, Side)
	float DeltaYaw = FRotator::NormalizeAxis(VelocityRotation.Yaw - CameraRotation.Yaw);

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

void AGGJCharacter::Move(const FInputActionValue& Value)
{
	// Input is Vector2D (X = Right/Left, Y = Forward/Backward)
	FVector2D MovementVector = Value.Get<FVector2D>();
	
	// Call the logic function
	ApplyMovementInput(MovementVector, false);
}

void AGGJCharacter::ApplyMovementInput(FVector2D MovementVector, bool IgnoreState)
{
	if (!IgnoreState && ActionState != ECharacterActionState::None) return;
	
	if (Controller != nullptr)
	{
		// Calculate a movement direction based on Camera rotation.
		// This ensures controls remain intuitive (W is always "Up" on screen) even if the camera rotates.
		const FRotator Rotation = FollowCamera->GetComponentRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		AddMovementInput(ForwardDirection, MovementVector.Y);
		AddMovementInput(RightDirection, MovementVector.X);
	}
}

#pragma endregion

#pragma region Combat Logic

void AGGJCharacter::OnHitboxOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// Ignore self
	if (OtherActor == this) return;

	// Check if we hit a Hurtbox (usually you check for a specific Tag or Interface)
	if (OtherComp && OtherComp->ComponentHasTag(TEXT("Hurtbox")))
	{
		// Calculate damage based on current combo index
		float DamageToDeal = 10.0f; // Fallback default
		if (ComboDamageValues.IsValidIndex(AttackComboIndex))
		{
			DamageToDeal = ComboDamageValues[AttackComboIndex];
		}

		// Apply Charge Multiplier
		DamageToDeal *= CurrentDamageMultiplier;

		// Apply Damage
		// Controller is the Instigator (who caused it), this is the DamageCauser
		UGameplayStatics::ApplyDamage(OtherActor, DamageToDeal, GetController(), this, UDamageType::StaticClass());
	}
}

float AGGJCharacter::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	// Call the base class - important for internal engine logic
	float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	// If already dead or invincible, don't take damage
	if (ActionState == ECharacterActionState::Dead || bIsInvincible)
	{
		return 0.0f;
	}

	if (ActualDamage > 0.0f)
	{
		// Reduce Health
		CurrentHealth = FMath::Clamp(CurrentHealth - ActualDamage, 0.0f, MaxHealth);

		if (CurrentHealth > 0.0f)
		{
			// --- HIT REACTION LOGIC ---
			
			// 1. Enter Hurt State (Stops movement and attacks)
			ActionState = ECharacterActionState::Hurt;
			
			// 2. Interrupt any ongoing Attack Combo
			GetWorld()->GetTimerManager().ClearTimer(ComboTimerHandle);
			DeactivateMeleeHitbox(); // Ensure hitbox is off
			bPendingCombo = false;

			// 3. Apply Knockback (Push character away from damage source)
			if (DamageCauser)
			{
				FVector KnockbackDir = (GetActorLocation() - DamageCauser->GetActorLocation()).GetSafeNormal2D();
				LaunchCharacter(KnockbackDir * KnockbackStrength, true, true);
			}

			// 4. Set Invincibility (I-Frames)
			bIsInvincible = true;
			GetWorld()->GetTimerManager().SetTimer(InvincibilityTimerHandle, this, &AGGJCharacter::DisableInvincibility, InvincibilityDuration, false);

			// 5. Set Stun Timer (When to regain control)
			GetWorld()->GetTimerManager().SetTimer(StunTimerHandle, this, &AGGJCharacter::OnStunFinished, HitStunDuration, false);
		}

		// Debug Log
		// UE_LOG(LogTemp, Warning, TEXT("Player took %f damage. Health: %f"), ActualDamage, CurrentHealth);

		if (CurrentHealth <= 0.0f)
		{
			// Handle Death
			ActionState = ECharacterActionState::Dead;
			// TODO: Disable Input, Play Death Animation, Show Game Over Screen
		}
	}

	return ActualDamage;
}

void AGGJCharacter::OnStunFinished()
{
	if (ActionState != ECharacterActionState::Dead)
	{
		ActionState = ECharacterActionState::None;
	}
}

void AGGJCharacter::DisableInvincibility()
{
	bIsInvincible = false;
	// Optional: Stop flashing visual effect here
}

void AGGJCharacter::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);

	// Reset combo index and cancel any pending combo timer when landing
	AttackComboIndex = 0;
	GetWorld()->GetTimerManager().ClearTimer(ComboTimerHandle);
	bPendingCombo = false;
}

#pragma endregion

#pragma region Lunge & Aim Assist

AActor* AGGJCharacter::FindBestTarget(FVector InputDirection)
{
	// If no input, use the direction the character is visually facing
	if (InputDirection.IsNearlyZero())
	{
		FRotator CameraRotation = FollowCamera->GetComponentRotation();
		float TargetYaw = CameraRotation.Yaw + AnimDirection;
		InputDirection = FRotator(0.0f, TargetYaw, 0.0f).Vector();
	}
	else
	{
		// Normalizziamo per sicurezza
		InputDirection.Normalize();
	}
	
	FVector StartLoc = GetActorLocation();
	
	// Use Sphere Overlap instead of Sweep to find all candidates around us
	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);

	GetWorld()->OverlapMultiByChannel(
		Overlaps,
		StartLoc,
		FQuat::Identity,
		ECC_Pawn,
		FCollisionShape::MakeSphere(LungeRange),
		Params
	);

	AActor* BestTarget = nullptr;
	float BestDistanceSq = FLT_MAX;
	
	// Convert angle to Cosine for Dot Product (performance optimization)
	// Example: 45 degrees -> 0.707. If Dot > 0.707, it's inside the cone.
	float MinDotProduct = FMath::Cos(FMath::DegreesToRadians(LungeHalfAngle));

	for (const FOverlapResult& Overlap : Overlaps)
	{
		if (AEnemyCharacter* Enemy = Cast<AEnemyCharacter>(Overlap.GetActor()))
		{
			// 1. Calculate direction to enemy
			FVector DirToEnemy = (Enemy->GetActorLocation() - StartLoc);
			float DistSq = DirToEnemy.SizeSquared(); // Use Squared to avoid square roots
			DirToEnemy.Normalize();

			// 2. Angle Check (Aim Assist Cone)
			// Compare Input Direction with Enemy Direction
			float Dot = FVector::DotProduct(InputDirection, DirToEnemy);

			if (Dot >= MinDotProduct)
			{
				// 3. Distance Check (Pick the closest valid one)
				if (DistSq < BestDistanceSq)
				{
					BestDistanceSq = DistSq;
					BestTarget = Enemy;
				}
			}
		}
	}
	
	return BestTarget;
}

void AGGJCharacter::PerformLunge(AActor* Target)
{
	if (!Target) return;
	
	// 1. Calculate Static Distance (Current Position) to estimate duration
	float Distance = FVector::Dist(GetActorLocation(), Target->GetActorLocation());
	
	float MyRadius = GetCapsuleComponent()->GetScaledCapsuleRadius();
	float TargetRadius = 30.0f; // Fallback default
	
	if (ACharacter* TargetChar = Cast<ACharacter>(Target))
	{
		TargetRadius = TargetChar->GetCapsuleComponent()->GetScaledCapsuleRadius();
	}

	// Safety buffer to avoid clipping
	float DynamicStopDistance = MyRadius + TargetRadius + 10.0f;
	float ActualStopDistance = FMath::Max(LungeStopDistance, DynamicStopDistance);
	
	// Check static distance BEFORE prediction.
	// If we are already physically close to the target NOW, do not lunge, even if they are running away.
	// Just rotate towards them and attack in place.
	// Buffer increased to 25.0f to avoid micro-jitters when already in attack range.
	if (Distance <= ActualStopDistance + 25.0f)
	{
		FVector DirToTarget = (Target->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
		if (!DirToTarget.IsNearlyZero())
		{
			SetActorRotation(DirToTarget.Rotation());
		}
		return;
	}

	// Estimate duration based on current distance
	float EstimatedDist = FMath::Max(0.0f, Distance - ActualStopDistance);
	float LungeDuration = FMath::Clamp(EstimatedDist / LungeSpeed, 0.15f, 0.4f);

	// 2. TARGET PREDICTION
	// If the enemy is moving, calculate where they will be at the end of the lunge.
	FVector TargetVelocity = Target->GetVelocity();
	TargetVelocity.Z = 0.0f; // Ignoriamo movimento verticale (salto nemico)

	FVector PredictedTargetLoc = Target->GetActorLocation() + (TargetVelocity * LungeDuration);

	// Ricalcoliamo la direzione verso il punto PREDETTO
	FVector DirectionToTarget = (PredictedTargetLoc - GetActorLocation()).GetSafeNormal();
	DirectionToTarget.Z = 0.0f;
	
	// Ruotiamo verso dove sarà il nemico
	SetActorRotation(DirectionToTarget.Rotation());

	// Recalculate distance to PREDICTED point
	float PredictedDistance = FVector::Dist(GetActorLocation(), PredictedTargetLoc);

	// JITTER FIX: If we will be close enough to the target (or they run into us), avoid lunge.
	if (PredictedDistance <= ActualStopDistance + 25.0f) return;

	// --- PHYSICS VELOCITY CALCULATION ON PREDICTED TARGET ---
	// Aim slightly "inside" the stop distance (Overshoot) to compensate for BrakingDeceleration
	// which would otherwise make us stop short. Physical collision will stop us at the right spot.
	// 25.0f is a sufficient margin to ensure contact.
	float DistanceToTravel = PredictedDistance - (ActualStopDistance - 25.0f);
	
	if (DistanceToTravel <= 0.0f) return;
	
	// Inverse physics formula for friction: V0 = (d * k) / (1 - exp(-k * t))
	// This ensures we cover EXACTLY the distance despite friction.
	float Friction = GetCharacterMovement()->GroundFriction;
	float RequiredSpeed;

	// Apply friction compensation ONLY if grounded.
	// If airborne, friction is different or zero.
	if (GetCharacterMovement()->IsMovingOnGround() && Friction > 0.0f)
	{
		float Denom = 1.0f - FMath::Exp(-Friction * LungeDuration);
		RequiredSpeed = (DistanceToTravel * Friction) / Denom;
	}
	else
	{
		RequiredSpeed = DistanceToTravel / LungeDuration;
	}

	// RELAXED CLAMP:
	// Initial velocity (V0) must be higher than average speed (LungeSpeed) to overcome initial friction.
	// Allow an initial peak up to double the configured speed to guarantee arrival.
	// If we limit V0 strictly to LungeSpeed, we will never reach distant targets.
	float FinalSpeed = FMath::Min(RequiredSpeed, LungeSpeed * 2.0f);
	
	FVector LaunchVelocity = DirectionToTarget * FinalSpeed;

	// Apply Launch.
	// bXYOverride = true replaces current velocity (immediate stop if moving elsewhere)
	// bZOverride = false maintains current gravity
	LaunchCharacter(LaunchVelocity, true, false);
}

#pragma endregion

#pragma region Hitbox Management

void AGGJCharacter::ActivateMeleeHitbox(FName SocketName, FVector Extent)
{
	// Safety Check: If socket doesn't exist on current sprite, warn and abort.
	if (!GetSprite()->DoesSocketExist(SocketName))
	{
		return; // Avoid attaching to root by mistake
	}

	// Move hitbox to the socket defined in the current Flipbook frame.
	// SnapToTargetNotIncludingScale keeps hitbox scale independent of sprite (avoids distortion).
	HitboxComponent->AttachToComponent(GetSprite(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, SocketName);
	
	HitboxComponent->SetBoxExtent(Extent);
	HitboxComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
}

void AGGJCharacter::DeactivateMeleeHitbox()
{
	HitboxComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

#pragma endregion

#pragma region Attack & Charge Logic

void AGGJCharacter::PerformAttack()
{
	// Allow attack if we are in None state OR if we are currently Charging.
	// If we are Rolling, Hurt, or Dead, we cannot attack.
	if (ActionState != ECharacterActionState::None && ActionState != ECharacterActionState::Charging) return;
	
	// Aim Assist Logic:
	// Find target based on player input (or facing direction if idle)
	AActor* LungeTarget = FindBestTarget(GetLastMovementInputVector());
	PerformLunge(LungeTarget);
	
	// LOGIC:
	// If the Combo Timer is active, it means we are within the window of a previous attack -> Advance Combo.
	// OR if we flagged a pending combo from StartCharging (because we paused the timer there).
	// If not, we are starting a new chain -> Reset to 0.
	if (GetWorld()->GetTimerManager().IsTimerActive(ComboTimerHandle) || bPendingCombo)
	{
		AttackComboIndex++;
		// Wrap around if we exceed the max combo count (optional, or just clamp)
		if (AttackComboIndex >= MaxComboCount)
		{
			AttackComboIndex = 0;
		}
	}
	else
	{
		AttackComboIndex = 0;
	}
	
	// Stop the reset timer because we are now executing an attack
	GetWorld()->GetTimerManager().ClearTimer(ComboTimerHandle);
	bPendingCombo = false; // Reset flag consumed

	// Set state to Attacking (AnimBP will pick up AttackComboIndex and play the correct anim)
	ActionState = ECharacterActionState::Attacking;
	
}

void AGGJCharacter::StartCharging()
{
	// Can only start charging from Idle/Run (None)
	if (ActionState != ECharacterActionState::None) return;

	ActionState = ECharacterActionState::Charging;
	CurrentChargeTime = 0.0f;
	CurrentDamageMultiplier = 1.0f;

	// Check if we are continuing a combo BEFORE clearing the timer.
	if (GetWorld()->GetTimerManager().IsTimerActive(ComboTimerHandle))
	{
		bPendingCombo = true;
	}
	else
	{
		bPendingCombo = false;
	}

	// IMPORTANT: Pause the combo timer!
	// If we hold the button for 2 seconds, we don't want the combo to reset to 0 while we are charging.
	GetWorld()->GetTimerManager().ClearTimer(ComboTimerHandle);
}

void AGGJCharacter::UpdateCharging(const FInputActionValue& Value)
{
	if (ActionState != ECharacterActionState::Charging) return;

	// Accumulate time
	CurrentChargeTime += GetWorld()->GetDeltaSeconds();

	// Optional: You can clamp it here for UI purposes, but for damage calc we clamp at the end
	// if (CurrentChargeTime > MaxChargeTime) CurrentChargeTime = MaxChargeTime;
}

void AGGJCharacter::FinishCharging()
{
	// If we were not charging (maybe interrupted by hit), do nothing
	if (ActionState != ECharacterActionState::Charging) return;

	// Calculate Multiplier based on how long we held the button
	// 0.0s -> 1.0x damage
	// MaxChargeTime -> MaxChargeDamageMultiplier
	float Alpha = FMath::Clamp(CurrentChargeTime / MaxChargeTime, 0.0f, 1.0f);
	CurrentDamageMultiplier = FMath::Lerp(1.0f, MaxChargeDamageMultiplier, Alpha);

	// Execute the attack
	PerformAttack();
}

void AGGJCharacter::OnAttackFinished()
{
	ActionState = ECharacterActionState::None;

	// Start the timer. If the player doesn't attack again within 'ComboWindowTime', the combo resets.
	GetWorld()->GetTimerManager().SetTimer(ComboTimerHandle, this, &AGGJCharacter::ResetCombo, ComboWindowTime, false);
}

void AGGJCharacter::ResetCombo()
{
	AttackComboIndex = 0;
}

#pragma endregion

#pragma region Jump Logic

void AGGJCharacter::StartJumpSequence()
{
	// Prevent jumping if we are doing a blocking action (Like Attacking or Rolling)
	if (ActionState != ECharacterActionState::None) return;

	// Reset combo index and cancel any pending combo timer when jumping
	AttackComboIndex = 0;
	GetWorld()->GetTimerManager().ClearTimer(ComboTimerHandle);

	// If the timer is already active, we are already waiting to jump. Do not restart the timer.
	if (GetWorld()->GetTimerManager().IsTimerActive(JumpTimerHandle)) return;

	bJumpStopPending = false;

	// If a delay is set, start the timer
	if (JumpDelayTime > 0.0f)
	{
		bStartJumping = true;
		GetWorld()->GetTimerManager().SetTimer(JumpTimerHandle, this, &AGGJCharacter::PerformJump, JumpDelayTime, false);
	}
	else
	{
		bStartJumping = true;
		// Otherwise jump immediately
		PerformJump();
	}
}

void AGGJCharacter::StopJumpSequence()
{
	// If the timer is still active, it means we released the button BEFORE the jump happened.
	// We mark it as pending so that when the jump finally happens, we immediately stop it (short hop).
	if (GetWorld()->GetTimerManager().IsTimerActive(JumpTimerHandle))
	{
		bJumpStopPending = true;
	}
	else
	{
		StopJumping();
	}
}

void AGGJCharacter::PerformJump()
{
	Jump();
	bStartJumping = false;

	// If the player released the button during the delay, we cut the jump short immediately
	if (bJumpStopPending)
	{
		// Use a small delay (0.1s) instead of NextTick to ensure the Jump input is processed 
		// by the CharacterMovementComponent before clearing it. 
		// NextTick can sometimes be too fast, causing the jump to be ignored.
		FTimerHandle StopJumpTimerHandle;
		GetWorld()->GetTimerManager().SetTimer(StopJumpTimerHandle, this, &ACharacter::StopJumping, 0.1f, false);
		bJumpStopPending = false;
	}
}

#pragma endregion

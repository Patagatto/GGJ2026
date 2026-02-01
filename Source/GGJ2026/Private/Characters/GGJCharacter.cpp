// Fill out your copyright notice in the Description page of Project Settings.


#include "Characters/GGJCharacter.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "PaperFlipbookComponent.h"
#include "Components/BoxComponent.h"
#include "TimerManager.h"
#include "Characters/EnemyCharacter.h"
#include "Engine/OverlapResult.h"
#include "Components/SphereComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Items/MaskPickup.h"
#include "PaperFlipbook.h"
#include "GameFramework/DamageType.h"
#include "InputMappingContext.h"


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
	HurtboxComponent->SetBoxExtent(FVector(20.f, 10.f, 40.f)); // Made it thinner
	// Set the hurtbox to be of the "Pawn" object type
	HurtboxComponent->SetCollisionObjectType(ECC_Pawn);
	// It should generate overlap events but not block anything by default
	HurtboxComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	HurtboxComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	HurtboxComponent->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);
	HurtboxComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	HurtboxComponent->SetCollisionResponseToChannel(ECC_GameTraceChannel3, ECR_Overlap);
	HurtboxComponent->SetCollisionResponseToChannel(ECC_GameTraceChannel4, ECR_Overlap);
	HurtboxComponent->SetGenerateOverlapEvents(true);

	MaskSprite = CreateDefaultSubobject<UPaperFlipbookComponent>(TEXT("MaskSprite"));
	MaskSprite->SetupAttachment(GetSprite());
	MaskSprite->SetCollisionEnabled(ECollisionEnabled::NoCollision); // Purely visual
	// Add a small forward offset to prevent clipping (Z-fighting) with the main sprite.
	MaskSprite->SetRelativeLocation(FVector(0.0f, 1.0f, 0.0f));
	
	// Hitbox: Deals damage
	HitboxComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("Hitbox"));
	// IMPORTANT: Attach to Sprite, not Root.
	// This ensures the hitbox moves correctly when the sprite is flipped (Scale X = -1).
	HitboxComponent->SetupAttachment(GetSprite()); 
	HitboxComponent->SetBoxExtent(FVector(30.f, 30.f, 30.f));
	// Set the hitbox to use our custom "PlayerHitbox" channel
	HitboxComponent->SetCollisionObjectType(ECC_GameTraceChannel3);
	// It should ignore everything by default...
	HitboxComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	// ...except for Pawns, which it should overlap with.
	// We change this to overlap with the Enemy's Hurtbox channel, which is ECC_GameTraceChannel4
	HitboxComponent->SetCollisionResponseToChannel(ECC_GameTraceChannel4, ECR_Overlap);
	HitboxComponent->SetGenerateOverlapEvents(false);
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
	
	// --- Interaction Setup ---
	InteractionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionSphere"));
	InteractionSphere->SetupAttachment(RootComponent);
	InteractionSphere->SetSphereRadius(100.0f);
	InteractionSphere->SetCollisionProfileName(TEXT("Trigger"));
	InteractionSphere->OnComponentBeginOverlap.AddDynamic(this, &AGGJCharacter::OnInteractionSphereOverlapBegin);
	InteractionSphere->OnComponentEndOverlap.AddDynamic(this, &AGGJCharacter::OnInteractionSphereOverlapEnd);
	
#pragma endregion
}

void AGGJCharacter::BeginPlay()
{
	Super::BeginPlay();
	
	// Unreal defaults to Falling on spawn. We force Walking so the first frame 
	// doesn't play the fall animation if we are spawning on the ground.
	// If we are actually in the air, the next physics update will correct it to Falling.
	GetCharacterMovement()->SetMovementMode(MOVE_Walking);

	// Save the default braking deceleration (drag) so we can restore it after rolling
	DefaultBrakingDeceleration = GetCharacterMovement()->BrakingDecelerationWalking;
	// Save the default roll cooldown and walk speed so we can restore them after buffs expire
	DefaultRollCooldown = RollCooldown;
	DefaultMaxWalkSpeed = GetCharacterMovement()->MaxWalkSpeed;
	
	// Center sprite relative to capsule to prevent visual "orbiting" (depth shifting) during rotation.
	// Keeps Z (height) as set in Blueprint.
	GetSprite()->SetRelativeLocation(FVector(0.0f, 0.0f, GetSprite()->GetRelativeLocation().Z));

	// Initialize Health
	CurrentHealth = 50;  //MaxHealth;

	// Reset Combat States on Spawn
	ActionState = ECharacterActionState::None;
	AttackComboIndex = 0;
	CurrentDamageMultiplier = 1.0f;
	bJumpStopPending = false;
	bPendingCombo = false;
	bIsRollOnCooldown = false;
	bExtendsDurationOnHit = false;
	bHasDamageReduction = false;
	bIsImmuneToKnockdown = false;
	bHasLifesteal = false;
	OverlappingMask = nullptr;
	bInputConsumed = false;

	// Initialize LastFacingDirection based on the initial AnimDirection (180) and Camera Rotation
	// This ensures we start with a valid direction even before moving
	const FRotator CameraRotation = GetCameraRotation();
	const float InitialYaw = CameraRotation.Yaw + AnimDirection;
	LastFacingDirection = FRotator(0.0f, InitialYaw, 0.0f).Vector();
}

void AGGJCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Handle Lunge movement interpolation
	if (bIsLunging)
	{
		const float LungeDuration = 0.15f;
		SetActorLocation(FMath::VInterpTo(GetActorLocation(), LungeTargetLocation, DeltaSeconds, 1.0f / LungeDuration)); 
	}

	// Calculate speed and movement state for AnimBP
	Speed = GetVelocity().Size2D();
	bIsMoving = Speed > 1.0f;

	// Update jumping state (True if in air)
	bIsJumping = GetCharacterMovement()->IsFalling();
	VerticalVelocity = GetVelocity().Z;
	
	// DEBUG: Print values to screen to verify C++ is working
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Yellow, FString::Printf(TEXT("Speed: %.2f | MaxSpeed: %.0f | Moving: %d | Dir: %.2f | Jumping:%d | Z: %.2f"), Speed, GetCharacterMovement()->MaxWalkSpeed, bIsMoving, AnimDirection, bIsJumping, VerticalVelocity));
		GEngine->AddOnScreenDebugMessage(3, 0.0f, FColor::Green, FString::Printf(TEXT("Current Health: %.1f"), CurrentHealth));
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

		// Roll
		EnhancedInputComponent->BindAction(RollAction, ETriggerEvent::Started, this, &AGGJCharacter::PerformRoll);

		// Interact
		EnhancedInputComponent->BindAction(InteractAction, ETriggerEvent::Started, this, &AGGJCharacter::Interact);
		// LauncheMask
		EnhancedInputComponent->BindAction(InteractAction, ETriggerEvent::Triggered, this, &AGGJCharacter::ChargeMask);
		EnhancedInputComponent->BindAction(InteractAction, ETriggerEvent::Completed, this, &AGGJCharacter::LaunchMask);
	}
}

void AGGJCharacter::PawnClientRestart()
{
	// Super::PawnClientRestart() is important as it calls SetupPlayerInputComponent.
	Super::PawnClientRestart();

	// This is the ideal place to set up input contexts, as it's called for every player-controlled pawn
	// when it's possessed by its controller on the client.
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			// Clear any existing mappings to prevent stacking them on re-possess
			Subsystem->ClearAllMappings();

			const int32 PlayerIndex = PlayerController->GetLocalPlayer()->GetControllerId();

			// For local gamepad multiplayer, all players use the same mapping context.
			if (DefaultMappingContext)
			{
				Subsystem->AddMappingContext(DefaultMappingContext, 0);
				UE_LOG(LogTemp, Log, TEXT("PawnClientRestart: Applied Input Mapping Context '%s' to player %d (%s)."), *DefaultMappingContext->GetName(), PlayerIndex, *GetName());
			}
			else
			{
				// This log is critical for debugging. If it fires, it means the correct character Blueprint isn't being spawned.
				UE_LOG(LogTemp, Error, TEXT("PawnClientRestart: DefaultMappingContext is NULL for player %d. Check the Character Blueprint!"), PlayerIndex);
			}
		}
	}
}

#pragma region Movement Logic

FRotator AGGJCharacter::GetCameraRotation() const
{
	// Since we are using a Shared Camera (ViewTarget), we should query the PlayerCameraManager.
	// In local multiplayer, Player 0's camera manager usually dictates the view if sharing screen,
	// or we can use GetControlRotation() if the controller is updated by the view target.
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		return PC->PlayerCameraManager ? PC->PlayerCameraManager->GetCameraRotation() : FRotator(-45.f, -90.f, 0.f);
	}
	// Fallback if no controller yet
	return FRotator(-45.f, -90.f, 0.f);
}

void AGGJCharacter::UpdateAnimationDirection()
{
	const FVector Velocity = GetVelocity();

	// Update LastFacingDirection from Velocity if moving significantly.
	// If not moving, LastFacingDirection retains the last Input (updated in ApplyMovementInput).
	if (Velocity.SizeSquared2D() > 1.0f)
	{
		LastFacingDirection = Velocity.GetSafeNormal2D();
	}
	
	const FRotator CameraRotation = GetCameraRotation();
	const FRotator TargetRotation = LastFacingDirection.ToOrientationRotator();

	// Calculate the angle difference between the camera direction and velocity direction
	// This allows the AnimBP to select the correct directional animation (Front, Back, Side)
	const float DeltaYaw = FRotator::NormalizeAxis(TargetRotation.Yaw - CameraRotation.Yaw);

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
	if (Controller != nullptr)
	{
		// Calculate a movement direction based on Camera rotation.
		// This ensures controls remain intuitive (W is always "Up" on screen) even if the camera rotates.
		const FRotator Rotation = GetCameraRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// Update LastFacingDirection based on Raw Input
		// Even if movement is blocked (e.g. Attacking), we want to know where the player is aiming.
		const FVector WorldInput = (ForwardDirection * MovementVector.Y) + (RightDirection * MovementVector.X);
		if (WorldInput.SizeSquared() > 0.01f)
		{
			LastFacingDirection = WorldInput.GetSafeNormal();
		}

		// Standard Action Game Roll: You commit to the direction. No steering during the roll.
		if (!IgnoreState && ActionState != ECharacterActionState::None) return;

		// Prevent movement while charging a mask throw
		if (ActionState == ECharacterActionState::ChargeMask) return;

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

	// If this actor has already been hit during this swing, ignore it.
	if (HitActors.Contains(OtherActor)) return;

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
		
		
		// If wearing a mask, extend its duration
		if (bExtendsDurationOnHit)
		{
			ExtendMaskDuration();
		}

		// If Red Rabbit mask is active (Lifesteal), heal the player
		if (bHasLifesteal)
		{
			CurrentHealth = FMath::Clamp(CurrentHealth + (DamageToDeal * RedRabbit_LifestealAmount), 0.0f, MaxHealth);
		}

		// Add the actor to the hit list for this swing.
		HitActors.Add(OtherActor);

		// Debug Log
		// UE_LOG(LogTemp, Warning, TEXT("Hit %s for %f damage"), *OtherActor->GetName(), DamageToDeal);
	}
}

void AGGJCharacter::OnStunFinished()
{
	// Only return to None if we are still in the Hurt state.
	// If we transitioned to KnockedDown or Dead, this timer shouldn't override it.
	if (ActionState == ECharacterActionState::Hurt)
	{
		ActionState = ECharacterActionState::None;
	}
}

void AGGJCharacter::OnGroundedTimerFinished()
{
	if (ActionState == ECharacterActionState::Grounded)
	{
		ActionState = ECharacterActionState::GettingUp;
		
		// Trigger Blueprint event for visuals
		OnGettingUp();
	}
}

void AGGJCharacter::ResetHitCount()
{
	CurrentHitCount = 0;
}

void AGGJCharacter::OnGetUpFinished()
{
	if (ActionState == ECharacterActionState::GettingUp)
	{
		ActionState = ECharacterActionState::None;
		DisableInvincibility();
	}
}

void AGGJCharacter::DisableInvincibility()
{
	bIsInvincible = false;
	// Optional: Stop flashing visual effect here
}

void AGGJCharacter::InterruptCombatActions()
{
	// If we are currently charging, ensure the charge end event is fired (stops sounds/VFX)
	if (ActionState == ECharacterActionState::Charging)
	{
		OnChargeEnded();
	}

	// Stop any active combo timers
	GetWorld()->GetTimerManager().ClearTimer(ComboTimerHandle);
	
	// Disable hitboxes
	DeactivateMeleeHitbox();
	
	// Reset flags
	bPendingCombo = false;
	bIsLunging = false;
	CurrentChargeTime = 0.0f;
}

void AGGJCharacter::HandleKnockdown(AActor* DamageCauser)
{
	// Reset hit counter
	CurrentHitCount = 0;
	ActionState = ECharacterActionState::KnockedDown;

	// We've been knocked down, so the hit combo is over. Clear the reset timer.
	GetWorld()->GetTimerManager().ClearTimer(HitCountResetTimerHandle);

	// Clear any pending stun recovery, as we are now in a full knockdown sequence.
	GetWorld()->GetTimerManager().ClearTimer(StunTimerHandle);

	// Become invincible for the whole sequence (will be disabled in OnGetUpFinished)
	bIsInvincible = true;

	// Trigger Blueprint Event for sound/VFX
	OnKnockedDown();

	FVector KnockbackDir;

	if (DamageCauser)
	{
		KnockbackDir = (GetActorLocation() - DamageCauser->GetActorLocation()).GetSafeNormal2D();

		// Force visual facing direction towards the enemy
		LastFacingDirection = -KnockbackDir;
		UpdateAnimationDirection();
	}
	else
	{
		// Fallback: Knockback opposite to current facing if damage source is unknown
		KnockbackDir = -LastFacingDirection;
	}

	// Stop any previous movement (walking/sliding) to ensure the launch vector is clean
	GetCharacterMovement()->StopMovementImmediately();

	// Get pushed away from the enemy.
	// IMPORTANT: Add Z velocity (Vertical Bump) to force 'Falling' state.
	// This guarantees that the 'Landed' event will eventually fire to transition to 'Grounded'.
	FVector LaunchVelocity = KnockbackDir * KnockdownPushStrength;
	LaunchVelocity.Z = 400.0f; 

	LaunchCharacter(LaunchVelocity, true, true);
}

void AGGJCharacter::HandleHurt(AActor* DamageCauser)
{
	ActionState = ECharacterActionState::Hurt;

	// Apply Knockback
	if (DamageCauser)
	{
		const FVector KnockbackDir = (GetActorLocation() - DamageCauser->GetActorLocation()).GetSafeNormal2D();
		LaunchCharacter(KnockbackDir * KnockbackStrength, true, true);
	}

	// Set Stun Timer (When to regain control)
	GetWorld()->GetTimerManager().SetTimer(StunTimerHandle, this, &AGGJCharacter::OnStunFinished, HitStunDuration, false);

	// FIX: Enable temporary Invincibility after a normal hit.
	// This prevents getting hit 60 times a second inside an enemy hitbox and allows the Hurt animation to play.
	bIsInvincible = true;
	GetWorld()->GetTimerManager().SetTimer(InvincibilityTimerHandle, this, &AGGJCharacter::DisableInvincibility, InvincibilityDuration, false);

	// Start a timer to reset the hit counter if we don't get hit again soon.
	GetWorld()->GetTimerManager().ClearTimer(HitCountResetTimerHandle);
	GetWorld()->GetTimerManager().SetTimer(HitCountResetTimerHandle, this, &AGGJCharacter::ResetHitCount, HitCountResetTime, false);
}

void AGGJCharacter::HandleDeath()
{
	ActionState = ECharacterActionState::Dead;
	
	// 1. Disable Input immediately
	if (APlayerController* PC = Cast<APlayerController>(Controller))
	{
		DisableInput(PC);
	}

	// 2. Trigger Blueprint Event (Play Animation, Sound, VFX)
	OnPlayerDied();

	// 3. Slow Motion Effect (Dramatic Death)
	// In local multiplayer, this affects BOTH players as they share the world time.
	UGameplayStatics::SetGlobalTimeDilation(GetWorld(), 0.25f);

	// 4. Reset Time Dilation after a short delay
	// Note: Timers run on Game Time. If dilation is 0.25, 0.5 seconds game time = 2.0 seconds real time.
	GetWorld()->GetTimerManager().SetTimer(TimeDilationTimerHandle, this, &AGGJCharacter::ResetGlobalTimeDilation, 0.5f, false);
}

void AGGJCharacter::ResetGlobalTimeDilation()
{
	UGameplayStatics::SetGlobalTimeDilation(GetWorld(), 1.0f);
}

float AGGJCharacter::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	// Call the base class - important for internal engine logic
	float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	// If already dead, rolling, or invincible, don't take damage
	if (ActionState == ECharacterActionState::Dead || ActionState == ECharacterActionState::Rolling ||
		ActionState == ECharacterActionState::KnockedDown || ActionState == ECharacterActionState::Grounded ||
		ActionState == ECharacterActionState::GettingUp ||
		bIsInvincible)
	{
		return 0.0f;
	}

	// Apply Damage Reduction if the buff is active
	if (bHasDamageReduction)
	{
		ActualDamage *= (1.0f - GreenBird_DamageReductionAmount);
	}

	// If no damage is taken, return early
	if (ActualDamage <= 0.0f) return 0.0f;

	// Reduce Health
	CurrentHealth = FMath::Clamp(CurrentHealth - ActualDamage, 0.0f, MaxHealth);

	if (CurrentHealth <= 0.0f)
	{
		HandleDeath();
		return ActualDamage;
	}

	// --- HIT REACTION LOGIC ---
	InterruptCombatActions();

	CurrentHitCount++;

	if (CurrentHitCount >= HitsUntilKnockdown && !bIsImmuneToKnockdown)
	{
		HandleKnockdown(DamageCauser);
	}
	else
	{
		HandleHurt(DamageCauser);
	}

	return ActualDamage;
}

void AGGJCharacter::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);

	if (ActionState == ECharacterActionState::KnockedDown)
	{
		ActionState = ECharacterActionState::Grounded;
		GetWorld()->GetTimerManager().SetTimer(GroundedTimerHandle, this, &AGGJCharacter::OnGroundedTimerFinished, GroundedTime, false);
	}
	else
	{
		// Reset combo index and cancel any pending combo timer when landing from a normal jump
		AttackComboIndex = 0;
		GetWorld()->GetTimerManager().ClearTimer(ComboTimerHandle);
		bPendingCombo = false;
	}
}

#pragma endregion

#pragma region Lunge & Aim Assist

AActor* AGGJCharacter::FindBestTarget(FVector InputDirection)
{
	FVector SearchDirection;

	// If there is significant player input, use that as the search direction.
	if (InputDirection.SizeSquared() > 0.01f)
	{
		SearchDirection = InputDirection.GetSafeNormal();
	}
	// Otherwise, use the visual facing direction of the sprite (Left or Right).
	else 
	{
		const FRotator CamRot = GetCameraRotation();
		FVector CameraRight = FRotationMatrix(CamRot).GetUnitAxis(EAxis::Y);
		CameraRight.Z = 0.0f;
		CameraRight.Normalize();

		// If Sprite is flipped, it's looking Left relative to the Camera.
		if (GetSprite()->GetRelativeScale3D().X < 0.0f)
		{
			SearchDirection = -CameraRight;
		}
		else
		{
			SearchDirection = CameraRight;
		}
	}
	
	FVector StartLoc = GetActorLocation();
	// Use FCollisionQueryParams to ignore self efficiently
	TArray<FOverlapResult> OverlapResults;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);

	GetWorld()->OverlapMultiByChannel(
		OverlapResults,
		StartLoc,
		FQuat::Identity,
		ECC_GameTraceChannel3,
		FCollisionShape::MakeSphere(LungeRange),
		Params
	);

	AActor* BestTarget = nullptr;
	float BestDistanceSq = FLT_MAX;
	
	const float MinDotProduct = FMath::Cos(FMath::DegreesToRadians(LungeHalfAngle));

	for (const FOverlapResult& Overlap : OverlapResults)
	{
		if (AEnemyCharacter* Enemy = Cast<AEnemyCharacter>(Overlap.GetActor()))
		{
			FVector DirToEnemy = (Enemy->GetActorLocation() - StartLoc);
			float DistSq = DirToEnemy.SizeSquared();
			DirToEnemy.Normalize();

			float Dot = FVector::DotProduct(SearchDirection, DirToEnemy);

			if (Dot >= MinDotProduct)
			{
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
	if (!Target || bIsLunging) return;

	// --- 1. Determine Final Destination ---
	// Get the camera's right vector to define the "line" of combat.
	const FRotator CamRot = GetCameraRotation();
	FVector CameraRight = FRotationMatrix(CamRot).GetUnitAxis(EAxis::Y);
	CameraRight.Z = 0.0f;
	CameraRight.Normalize();

	// Determine if the player is currently on the left or right side of the target
	const FVector VectorToPlayer = GetActorLocation() - Target->GetActorLocation();
	const float SideProjection = FVector::DotProduct(VectorToPlayer, CameraRight);

	// Calculate the desired final destination point (left or right of the enemy)
	FVector DestinationPoint;
	if (SideProjection >= 0.0f)
	{
		// Player is on the right, so the destination is the point on the right.
		DestinationPoint = Target->GetActorLocation() + (CameraRight * LungeStopDistance);
	}
	else
	{
		// Player is on the left, so the destination is the point on the left.
		DestinationPoint = Target->GetActorLocation() - (CameraRight * LungeStopDistance);
	}

	// Ensure the destination stays on the same vertical level as the player to prevent "hopping"
	DestinationPoint.Z = GetActorLocation().Z;

	// --- 2. Check if Lunge is Necessary ---
	const FVector VectorToDestination = DestinationPoint - GetActorLocation();
	const float DistanceToDestination = VectorToDestination.Size2D();

	// If we are already very close to our destination, just rotate and attack.
	// This is the crucial "deadzone" check to prevent repeated lunges.
	if (DistanceToDestination <= 20.0f)
	{
		const FVector DirToActualTarget = (Target->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
		if (!DirToActualTarget.IsNearlyZero())
		{
			SetActorRotation(DirToActualTarget.Rotation());
		}
		return; // Do not lunge
	}

	// --- 3. Execute Lunge ---
	// Rotate to face the actual enemy, not the destination point
	const FVector DirToActualTarget = (Target->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
	if (!DirToActualTarget.IsNearlyZero())
	{
		SetActorRotation(DirToActualTarget.Rotation());
	}
	
	// Set the lunge state and target location. The Tick function will handle the movement.
	bIsLunging = true;
	LungeTargetLocation = DestinationPoint;
	
	// Disable physics-based movement during the lunge for full control
	GetCharacterMovement()->SetMovementMode(MOVE_Flying);
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
	
	// Clear the list of actors hit from the previous swing.
	HitActors.Empty();

	HitboxComponent->SetBoxExtent(Extent);
	HitboxComponent->SetGenerateOverlapEvents(true);
	HitboxComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
}

void AGGJCharacter::DeactivateMeleeHitbox()
{
	HitboxComponent->SetGenerateOverlapEvents(false);
	HitboxComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AGGJCharacter::ActivateMask(FName SocketName)
{
	// Safety Check: If socket doesn't exist on current sprite, warn and abort.
	if (!GetSprite()->DoesSocketExist(SocketName))
	{
		return; // Avoid attaching to root by mistake
	}

	// Move mask to the socket defined in the current Flipbook frame.
	MaskSprite->AttachToComponent(GetSprite(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, SocketName);

	// Re-apply the offset after attaching to the socket to prevent clipping.
	MaskSprite->SetRelativeLocation(FVector(0.0f, 1.0f, 0.0f));
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

	// Trigger Attack Started Event (Audio/VFX)
	const bool bHasMask = CurrentMaskType != EEnemyType::None;
	OnAttackStarted(false, bHasMask);
	
}

void AGGJCharacter::StartCharging()
{
	// Can only start charging from Idle/Run (None)
	if (ActionState != ECharacterActionState::None) return;

	ActionState = ECharacterActionState::Charging;
	CurrentChargeTime = 0.0f;
	CurrentDamageMultiplier = 1.0f;
	
	// Stop movement immediately to prevent sliding while charging
	GetCharacterMovement()->StopMovementImmediately();

	// Trigger Blueprint event for Charge Start (Audio/VFX)
	OnChargeStarted();

	// Check if we are continuing a combo BEFORE clearing the timer.
	if (GetWorld()->GetTimerManager().IsTimerActive(ComboTimerHandle))
	{
		bPendingCombo = true;
	}
	else
	{
		bPendingCombo = false;
	}
	
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

	// Trigger Blueprint event for Charge End (Stop Audio/VFX)
	OnChargeEnded();

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
	// Trigger Attack Completed Event with results
	const bool bHitEnemy = HitActors.Num() > 0;
	const bool bHasMask = CurrentMaskType != EEnemyType::None;
	OnAttackCompleted(bHitEnemy, bHasMask);

	ActionState = ECharacterActionState::None;

	// If we were lunging, stop it and restore normal movement physics
	if (bIsLunging)
	{
		bIsLunging = false;
		GetCharacterMovement()->SetMovementMode(MOVE_Walking);
		GetCharacterMovement()->BrakingDecelerationWalking = DefaultBrakingDeceleration;
	}

	// Start the timer. If the player doesn't attack again within 'ComboWindowTime', the combo resets.
	GetWorld()->GetTimerManager().SetTimer(ComboTimerHandle, this, &AGGJCharacter::ResetCombo, ComboWindowTime, false);
}
 
void AGGJCharacter::ResetCombo()
{
	AttackComboIndex = 0;
}

void AGGJCharacter::PerformRoll()
{
	// Cannot roll if Dead, Hurt, already Rolling, or on cooldown.
	// Can roll from None, Attacking, Charging (Interrupting them).
	if (ActionState == ECharacterActionState::Dead || 
		ActionState == ECharacterActionState::Hurt || 
		ActionState == ECharacterActionState::Rolling ||
		bIsRollOnCooldown)
	{
		return;
	}

	// INTERRUPT LOGIC:
	// If we are attacking or charging, we must cancel everything.
	if (ActionState == ECharacterActionState::Attacking || ActionState == ECharacterActionState::Charging)
	{
		InterruptCombatActions();
	}

	// Determine Direction
	FVector RollDirection = GetLastMovementInputVector();
	RollDirection.Z = 0.0f; // Force planar movement to prevent vertical drift
	
	// Handle Stick Drift / Small Input:
	// If the input is very small (magnitude < 0.1), treat it as zero so we use the character's facing direction instead.
	if (RollDirection.SizeSquared() < 0.01f)
	{
		// If standing still, roll in the direction we are facing visually.
		// Since the sprite is 2D and flips Left/Right, we use the Camera's Right vector.
		
		const FRotator CamRot = GetCameraRotation();
		FVector CameraRight = FRotationMatrix(CamRot).GetUnitAxis(EAxis::Y);
		CameraRight.Z = 0.0f; // Ensure planar movement
		CameraRight.Normalize();

		// If Sprite is flipped (Scale X < 0), it means we are looking Left relative to Camera.
		if (GetSprite()->GetRelativeScale3D().X < 0.0f)
		{
			RollDirection = -CameraRight;
		}
		else
		{
			RollDirection = CameraRight;
		}
	}
	else
	{
		RollDirection.Normalize();
	}

	// Rotate character to face roll direction immediately
	SetActorRotation(RollDirection.Rotation());

	// Physics Adjustment:
	// Normally, CharacterMovement brakes hard when you stop input.
	// We disable braking during the roll so the Launch impulse carries us smoothly
	// for the entire duration without needing player input.
	GetCharacterMovement()->BrakingDecelerationWalking = 0.0f;
	GetCharacterMovement()->GroundFriction = 2.0f; // Lower friction to slide farther
	// Override Z (true) to 0.0f to lock vertical movement and prevent hopping/diving on slopes
	LaunchCharacter(RollDirection * RollSpeed, true, true); 

	// Set State (Invincibility is handled in TakeDamage by checking this state)
	ActionState = ECharacterActionState::Rolling;

	// Start cooldown
	bIsRollOnCooldown = true;
	GetWorld()->GetTimerManager().SetTimer(RollCooldownTimerHandle, this, &AGGJCharacter::ResetRollCooldown, RollCooldown, false);
}

void AGGJCharacter::OnRollFinished()
{
	if (ActionState == ECharacterActionState::Rolling)
	{
		ActionState = ECharacterActionState::None;
		
		// Restore normal braking so we stop when releasing keys
		GetCharacterMovement()->GroundFriction = 3.0f; // Restore original friction
		GetCharacterMovement()->BrakingDecelerationWalking = DefaultBrakingDeceleration;
	}
}

void AGGJCharacter::ResetRollCooldown()
{
	bIsRollOnCooldown = false;
}

void AGGJCharacter::Interact()
{
	// If we have a valid mask to pick up
	if (OverlappingMask)
	{
		// If the mask is flying (thrown by someone), catch it!
		if (OverlappingMask->IsFlying())
		{
			OnMaskCaught();
			// Catching logic is the same as equipping
			EquipMask(OverlappingMask);
			bInputConsumed = true; // Prevent throwing immediately after catching
			return;
		}
		EquipMask(OverlappingMask);
		OnMaskPickUp();
		bInputConsumed = true; // Prevent throwing immediately after pickup
	}
}

void AGGJCharacter::ChargeMask()
{
	// If we just picked up the mask with this press, ignore the charge
	if (bInputConsumed) return;

	// Priority Check: If we are standing on a mask, we should interact, not charge.
	// This prevents ChargeMask from running if it fires before Interact in the same frame.
	if (OverlappingMask) return;

	if (CurrentMaskType == EEnemyType::None) return;
	if (ActionState != ECharacterActionState::None && ActionState != ECharacterActionState::ChargeMask) return;
	
	// Set state to stop movement
	ActionState = ECharacterActionState::ChargeMask;
	GetCharacterMovement()->StopMovementImmediately();

	// Optional: Rotate character towards input while charging
	if (GetLastMovementInputVector().SizeSquared() > 0.01f)
	{
		LastFacingDirection = GetLastMovementInputVector().GetSafeNormal();
		UpdateAnimationDirection();
	}
}

void AGGJCharacter::LaunchMask()
{
	// If we just picked up the mask, releasing the button should just reset the flag, not throw.
	if (bInputConsumed)
	{
		bInputConsumed = false;
		return;
	}

	if (ActionState != ECharacterActionState::ChargeMask) return;

	// Reset State
	ActionState = ECharacterActionState::None;

	if (CurrentMaskType == EEnemyType::None) return;

	// Spawn the mask pickup
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.Instigator = this;
	
	// Spawn slightly in front to avoid clipping
	FVector SpawnLoc = GetActorLocation() + (LastFacingDirection * 60.0f);
	SpawnLoc.Z += 40.0f; // Spawn higher from the ground
	
	AMaskPickup* ThrownMask = GetWorld()->SpawnActor<AMaskPickup>(AMaskPickup::StaticClass(), SpawnLoc, FRotator::ZeroRotator, SpawnParams);
	if (ThrownMask)
	{
		ThrownMask->MaskType = CurrentMaskType;
		ThrownMask->UpdateVisuals(RedRabbitMaskFlipbook, GreenBirdMaskFlipbook, BlueCatMaskFlipbook);
		ThrownMask->InitializeThrow(LastFacingDirection, this);
		
		// Remove mask from player (Unequip logic without destroying the actor we just spawned)
		UnequipMask(); 
	}
}

void AGGJCharacter::EquipMask(AMaskPickup* MaskToEquip)
{
	if (!MaskToEquip) return;

	// If already wearing a mask, remove the old one first
	if (CurrentMaskType != EEnemyType::None)
	{
		UnequipMask();
	}

	// Set new mask state
	CurrentMaskType = MaskToEquip->MaskType;
	CurrentMaskDuration = MaxMaskDuration;
	DrainRateMultiplier = 1.0f;
	OnMaskChanged(CurrentMaskType);

	// Apply the corresponding buff
	ApplyBuff(CurrentMaskType);

	// Start the timer that drains the mask's duration
	GetWorld()->GetTimerManager().SetTimer(MaskDurationTimerHandle, this, &AGGJCharacter::UpdateMaskDuration, 1.0f, true, 1.0f);
	
	// Set the visual representation of the mask
	UPaperFlipbook* FlipbookToSet = nullptr;
	switch (CurrentMaskType)
	{
		case EEnemyType::RedRabbit:
			FlipbookToSet = RedRabbitMaskFlipbook;
			break;
		case EEnemyType::GreenBird:
			FlipbookToSet = GreenBirdMaskFlipbook;
			break;
		case EEnemyType::BlueCat:
			FlipbookToSet = BlueCatMaskFlipbook;
			break;
		default: break;
	}
	MaskSprite->SetFlipbook(FlipbookToSet);

	// Destroy the pickup actor from the world and clear the reference
	MaskToEquip->Destroy();
	OverlappingMask = nullptr;
}

void AGGJCharacter::UnequipMask()
{
	if (CurrentMaskType == EEnemyType::None) return;

	// Remove the buff
	RemoveBuff(CurrentMaskType);

	// Reset state
	CurrentMaskType = EEnemyType::None;
	CurrentMaskDuration = 0.0f;
	GetWorld()->GetTimerManager().ClearTimer(MaskDurationTimerHandle);
	MaskSprite->SetFlipbook(nullptr); // Hide the mask by removing its flipbook
	OnMaskChanged(CurrentMaskType);
	
	// Note: We do NOT destroy the pickup here if called from LaunchMask, 
	// because LaunchMask spawns a NEW actor. This function just clears local state.
}

void AGGJCharacter::UpdateMaskDuration()
{
	if (CurrentMaskType == EEnemyType::None || ActionState == ECharacterActionState::Dead)
	{
		GetWorld()->GetTimerManager().ClearTimer(MaskDurationTimerHandle);
		return;
	}

	// Decrease duration by 1 second, scaled by the current drain multiplier
	CurrentMaskDuration -= (1.0f * DrainRateMultiplier);

	// Increase the drain rate for the next second, making it harder to maintain
	DrainRateMultiplier += DrainIncreaseRate;

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(1, 1.0f, FColor::Cyan, FString::Printf(TEXT("Mask Duration: %.1f s (Drain Rate: x%.2f)"), CurrentMaskDuration, DrainRateMultiplier));
	}

	if (CurrentMaskDuration <= 0.0f)
	{
		UnequipMask();
	}
}

void AGGJCharacter::ExtendMaskDuration()
{
	if (CurrentMaskType != EEnemyType::None)
	{
		CurrentMaskDuration = FMath::Clamp(CurrentMaskDuration + RedRabbit_TimeToAddOnHit, 0.0f, MaxMaskDuration);

		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(2, 1.5f, FColor::Green, FString::Printf(TEXT("HIT! Mask time extended to %.1f s"), CurrentMaskDuration));
		}
	}
}

void AGGJCharacter::ApplyBuff(EEnemyType MaskType)
{
	// Placeholder for buff logic
	switch (MaskType)
	{
		case EEnemyType::RedRabbit:
			bHasLifesteal = true;
			bExtendsDurationOnHit = true;
			UE_LOG(LogTemp, Warning, TEXT("Applied Red Rabbit Buff! Lifesteal and Mask Extension on hit enabled."));
			break;
		case EEnemyType::GreenBird:
			bIsImmuneToKnockdown = true;
			bHasDamageReduction = true;
			UE_LOG(LogTemp, Warning, TEXT("Applied Green Bird Buff! Knockdown immunity and Damage Reduction enabled."));
			break;
		case EEnemyType::BlueCat:
			RollCooldown = BlueCat_RollCooldown;
			GetCharacterMovement()->MaxWalkSpeed = BlueCat_MovementSpeed;
			UE_LOG(LogTemp, Warning, TEXT("Applied Blue Cat Buff! Roll cooldown reduced and movement speed increased."));
			break;
		default: break;
	}
}

void AGGJCharacter::RemoveBuff(EEnemyType MaskType)
{
	// Placeholder for removing buff logic
	switch (MaskType)
	{
		case EEnemyType::RedRabbit:
			bHasLifesteal = false;
			bExtendsDurationOnHit = false;
			UE_LOG(LogTemp, Warning, TEXT("Removed Red Rabbit Buff."));
			break;
		case EEnemyType::GreenBird:
			bIsImmuneToKnockdown = false;
			bHasDamageReduction = false;
			UE_LOG(LogTemp, Warning, TEXT("Removed Green Bird Buff."));
			break;
		case EEnemyType::BlueCat:
			RollCooldown = DefaultRollCooldown;
			GetCharacterMovement()->MaxWalkSpeed = DefaultMaxWalkSpeed;
			UE_LOG(LogTemp, Warning, TEXT("Removed Blue Cat Buff."));
			break;
		default: break;
	}
}

#pragma endregion

#pragma region Interaction Overlaps

void AGGJCharacter::OnInteractionSphereOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// Check if the overlapped actor is a mask pickup
	if (AMaskPickup* Mask = Cast<AMaskPickup>(OtherActor))
	{
		// Store a reference to it, so we know we can interact
		OverlappingMask = Mask;
		// TODO: Show "Interact" UI prompt
	}
}

void AGGJCharacter::OnInteractionSphereOverlapEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	// If we are no longer overlapping the mask we were targeting, clear the reference
	if (Cast<AMaskPickup>(OtherActor) == OverlappingMask)
	{
		OverlappingMask = nullptr;
		// TODO: Hide "Interact" UI prompt
	}
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

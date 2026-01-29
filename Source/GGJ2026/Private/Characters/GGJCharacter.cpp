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
#include "Components/SphereComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Items/MaskPickup.h"
#include "PaperFlipbook.h"
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
	HurtboxComponent->SetBoxExtent(FVector(20.f, 10.f, 40.f)); // Made it thinner
	// Set the hurtbox to be of the "Pawn" object type
	HurtboxComponent->SetCollisionObjectType(ECC_Pawn);
	// It should generate overlap events but not block anything by default
	HurtboxComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	HurtboxComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
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
	HitboxComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
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

	// --- Interaction Setup ---
	InteractionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionSphere"));
	InteractionSphere->SetupAttachment(RootComponent);
	InteractionSphere->SetSphereRadius(100.0f);
	InteractionSphere->SetCollisionProfileName(TEXT("Trigger"));
	InteractionSphere->OnComponentBeginOverlap.AddDynamic(this, &AGGJCharacter::OnInteractionSphereOverlapBegin);
	InteractionSphere->OnComponentEndOverlap.AddDynamic(this, &AGGJCharacter::OnInteractionSphereOverlapEnd);
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

	// Initialize Health
	CurrentHealth = MaxHealth;

	// Reset Combat States on Spawn
	ActionState = ECharacterActionState::None;
	AttackComboIndex = 0;
	CurrentDamageMultiplier = 1.0f;
	bJumpStopPending = false;
	bPendingCombo = false;
	bIsRollOnCooldown = false;
	OverlappingMask = nullptr;

	// Add Input Mapping Context
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}

	// Initialize LastFacingDirection based on the initial AnimDirection (180) and Camera Rotation
	// This ensures we start with a valid direction even before moving
	FRotator CameraRotation = FollowCamera->GetComponentRotation();
	float InitialYaw = CameraRotation.Yaw + AnimDirection;
	LastFacingDirection = FRotator(0.0f, InitialYaw, 0.0f).Vector();
}

void AGGJCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Handle Lunge movement over time
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

		// Roll
		EnhancedInputComponent->BindAction(RollAction, ETriggerEvent::Started, this, &AGGJCharacter::PerformRoll);

		// Interact
		EnhancedInputComponent->BindAction(InteractAction, ETriggerEvent::Started, this, &AGGJCharacter::Interact);
	}
}

#pragma region Movement Logic

void AGGJCharacter::UpdateAnimationDirection()
{
	FVector Velocity = GetVelocity();

	// Update LastFacingDirection from Velocity if moving significantly.
	// If not moving, LastFacingDirection retains the last Input (updated in ApplyMovementInput).
	if (Velocity.SizeSquared2D() > 1.0f)
	{
		LastFacingDirection = Velocity.GetSafeNormal2D();
	}
	
	FRotator CameraRotation = FollowCamera->GetComponentRotation();
	FRotator TargetRotation = LastFacingDirection.ToOrientationRotator();

	// Calculate the angle difference between the camera direction and velocity direction
	// This allows the AnimBP to select the correct directional animation (Front, Back, Side)
	float DeltaYaw = FRotator::NormalizeAxis(TargetRotation.Yaw - CameraRotation.Yaw);

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
		const FRotator Rotation = FollowCamera->GetComponentRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// Update LastFacingDirection based on Raw Input
		// Even if movement is blocked (e.g. Attacking), we want to know where the player is aiming.
		FVector WorldInput = (ForwardDirection * MovementVector.Y) + (RightDirection * MovementVector.X);
		if (WorldInput.SizeSquared() > 0.01f)
		{
			LastFacingDirection = WorldInput.GetSafeNormal();
		}

		// Standard Action Game Roll: You commit to the direction. No steering during the roll.
		if (!IgnoreState && ActionState != ECharacterActionState::None) return;

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
		ExtendMaskDuration();

		// Add the actor to the hit list for this swing.
		HitActors.Add(OtherActor);

		// Debug Log
		// UE_LOG(LogTemp, Warning, TEXT("Hit %s for %f damage"), *OtherActor->GetName(), DamageToDeal);
	}
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

float AGGJCharacter::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	// Call the base class - important for internal engine logic
	const float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	// If already dead, rolling, or invincible, don't take damage
	if (ActionState == ECharacterActionState::Dead || ActionState == ECharacterActionState::Rolling || bIsInvincible)
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
			
			// 2. Interrupt any ongoing Attack Combo or Lunge
			GetWorld()->GetTimerManager().ClearTimer(ComboTimerHandle);
			DeactivateMeleeHitbox();
			bPendingCombo = false;
			bIsLunging = false;

			// 3. Apply Knockback (Push character away from damage source)
			if (DamageCauser)
			{
				const FVector KnockbackDir = (GetActorLocation() - DamageCauser->GetActorLocation()).GetSafeNormal2D();
				LaunchCharacter(KnockbackDir * KnockbackStrength, true, true);
			}

			// 4. Set Invincibility (I-Frames)
			bIsInvincible = true;
			GetWorld()->GetTimerManager().SetTimer(InvincibilityTimerHandle, this, &AGGJCharacter::DisableInvincibility, InvincibilityDuration, false);

			// 5. Set Stun Timer (When to regain control)
			GetWorld()->GetTimerManager().SetTimer(StunTimerHandle, this, &AGGJCharacter::OnStunFinished, HitStunDuration, false);
		}
		else // CurrentHealth <= 0.0f
		{
			// Handle Death
			ActionState = ECharacterActionState::Dead;
			// TODO: Disable Input, Play Death Animation, Show Game Over Screen
		}
	}

	return ActualDamage;
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
	FVector SearchDirection;

	// If there is significant player input, use that as the search direction.
	if (InputDirection.SizeSquared() > 0.01f)
	{
		SearchDirection = InputDirection.GetSafeNormal();
	}
	// Otherwise, use the visual facing direction of the sprite (Left or Right).
	else 
	{
		FVector CameraRight = FollowCamera->GetRightVector();
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
	
	TArray<FOverlapResult> OverlapResults;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);

	GetWorld()->OverlapMultiByChannel(
		OverlapResults,
		StartLoc,
		FQuat::Identity,
		ECC_Pawn,
		FCollisionShape::MakeSphere(LungeRange),
		Params
	);

	AActor* BestTarget = nullptr;
	float BestDistanceSq = FLT_MAX;
	
	float MinDotProduct = FMath::Cos(FMath::DegreesToRadians(LungeHalfAngle));

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
	FVector CameraRight = FollowCamera->GetRightVector();
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
	HitboxComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
}

void AGGJCharacter::DeactivateMeleeHitbox()
{
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
		GetWorld()->GetTimerManager().ClearTimer(ComboTimerHandle);
		bPendingCombo = false;
		CurrentChargeTime = 0.0f;
		DeactivateMeleeHitbox();
		bIsLunging = false; // Stop any active lunge
	}

	// Determine Direction
	FVector RollDirection = GetLastMovementInputVector();
	
	// Handle Stick Drift / Small Input:
	// If the input is very small (magnitude < 0.1), treat it as zero so we use the character's facing direction instead.
	if (RollDirection.SizeSquared() < 0.01f)
	{
		// If standing still, roll in the direction we are facing visually.
		// Since the sprite is 2D and flips Left/Right, we use the Camera's Right vector.
		
		FVector CameraRight = FollowCamera->GetRightVector();
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
	LaunchCharacter(RollDirection * RollSpeed, true, false); // bXYOverride, bZOverride

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
		EquipMask(OverlappingMask);
	}
}

void AGGJCharacter::EquipMask(AMaskPickup* MaskToEquip)
{
	if (!MaskToEquip) return;

	// If already wearing a mask, remove the old one first
	if (CurrentMaskType != EMaskType::None)
	{
		UnequipMask();
	}

	// Set new mask state
	CurrentMaskType = MaskToEquip->MaskType;
	CurrentMaskDuration = MaxMaskDuration;
	DrainRateMultiplier = 1.0f;

	// Apply the corresponding buff
	ApplyBuff(CurrentMaskType);

	// Start the timer that drains the mask's duration
	GetWorld()->GetTimerManager().SetTimer(MaskDurationTimerHandle, this, &AGGJCharacter::UpdateMaskDuration, 1.0f, true, 1.0f);
	
	// Set the visual representation of the mask
	UPaperFlipbook* FlipbookToSet = nullptr;
	switch (CurrentMaskType)
	{
		case EMaskType::RedRabbit:
			FlipbookToSet = RedRabbitMaskFlipbook;
			break;
		case EMaskType::GreenBird:
			FlipbookToSet = GreenBirdMaskFlipbook;
			break;
		case EMaskType::BlueCat:
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
	if (CurrentMaskType == EMaskType::None) return;

	// Remove the buff
	RemoveBuff(CurrentMaskType);

	// Reset state
	CurrentMaskType = EMaskType::None;
	CurrentMaskDuration = 0.0f;
	GetWorld()->GetTimerManager().ClearTimer(MaskDurationTimerHandle);
	MaskSprite->SetFlipbook(nullptr); // Hide the mask by removing its flipbook

	UE_LOG(LogTemp, Warning, TEXT("Mask broke!"));
}

void AGGJCharacter::UpdateMaskDuration()
{
	if (CurrentMaskType == EMaskType::None || ActionState == ECharacterActionState::Dead)
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
	if (CurrentMaskType != EMaskType::None)
	{
		CurrentMaskDuration = FMath::Clamp(CurrentMaskDuration + TimeToAddOnHit, 0.0f, MaxMaskDuration);

		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(2, 1.5f, FColor::Green, FString::Printf(TEXT("HIT! Mask time extended to %.1f s"), CurrentMaskDuration));
		}
	}
}

void AGGJCharacter::ApplyBuff(EMaskType MaskType)
{
	// Placeholder for buff logic
	switch (MaskType)
	{
		case EMaskType::RedRabbit: UE_LOG(LogTemp, Warning, TEXT("Applied Red Rabbit Buff!")); break;
		case EMaskType::GreenBird: UE_LOG(LogTemp, Warning, TEXT("Applied Green Bird Buff!")); break;
		case EMaskType::BlueCat:   UE_LOG(LogTemp, Warning, TEXT("Applied Blue Cat Buff!"));   break;
		default: break;
	}
}

void AGGJCharacter::RemoveBuff(EMaskType MaskType)
{
	// Placeholder for removing buff logic
	switch (MaskType)
	{
		case EMaskType::RedRabbit: UE_LOG(LogTemp, Warning, TEXT("Removed Red Rabbit Buff.")); break;
		case EMaskType::GreenBird: UE_LOG(LogTemp, Warning, TEXT("Removed Green Bird Buff.")); break;
		case EMaskType::BlueCat:   UE_LOG(LogTemp, Warning, TEXT("Removed Blue Cat Buff."));   break;
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

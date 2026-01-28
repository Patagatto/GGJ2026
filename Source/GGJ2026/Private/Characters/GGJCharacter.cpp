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
#include "Kismet/GameplayStatics.h"
#include "GameFramework/DamageType.h"


AGGJCharacter::AGGJCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
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
	// IMPORTANTE: Attacchiamo allo Sprite, non alla Root. 
	// In questo modo se lo sprite viene flippato (Scale X = -1), l'hitbox si sposta correttamente.
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
	bJumpStopPending = false;

	// Add Input Mapping Context
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}
}

void AGGJCharacter::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);

	// Reset combo index and cancel any pending combo timer when landing
	AttackComboIndex = 0;
	GetWorld()->GetTimerManager().ClearTimer(ComboTimerHandle);
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
	}
}

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

AActor* AGGJCharacter::FindBestTarget(FVector InputDirection)
{
	if (InputDirection.IsNearlyZero())
	{
		InputDirection = GetActorForwardVector();
	}
	
	FVector StartLoc;
	FVector EndLoc;
	
	StartLoc = GetActorLocation();
	EndLoc = StartLoc + (InputDirection.GetSafeNormal() * 2000);
	
	FCollisionQueryParams CollisionParams;
	//CollisionParams.AddIgnoredActor(GetOwner());
	TArray<FHitResult> HitResults;
	
	bool HasHit = GetWorld()->SweepMultiByChannel(HitResults,StartLoc, EndLoc, FQuat::Identity, ECC_Pawn, FCollisionShape::MakeSphere(200.0f), CollisionParams);

	if (HasHit)
	{
		for (const FHitResult& HitResult : HitResults)
		{
			if (AGGJCharacter* Enemy = Cast<AGGJCharacter>(HitResult.GetActor()))
			{
				GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Yellow, FString::Printf(TEXT("Actor hit")));
				return Enemy;
			}
		}
	}
	return nullptr;
}

void AGGJCharacter::PerformLunge(AActor* Target)
{
	if (!Target) return;
	
	FVector DirectionToTarget = (Target->GetActorLocation() - GetActorLocation()).GetSafeNormal();
	
	DirectionToTarget.Z = 0.0f;
	
	FRotator LookAtRot = FRotationMatrix::MakeFromX(DirectionToTarget).Rotator();
	
	float Distance = FVector::Dist(GetActorLocation(), Target->GetActorLocation());
	float StopDistance = 100.0f;
	
	if (Distance <= StopDistance) return;
	
	FVector LaunchVelocity = DirectionToTarget * 1000.0f;
	
	LaunchCharacter(LaunchVelocity, true, true);
}

void AGGJCharacter::ActivateMeleeHitbox(FName SocketName, FVector Extent)
{
	// Controllo di sicurezza: Se il socket non esiste nello sprite corrente, avvisa!
	if (!GetSprite()->DoesSocketExist(SocketName))
	{
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red, FString::Printf(TEXT("MISSING SOCKET: %s on Sprite!"), *SocketName.ToString()));
		return; // Evita di attaccare alla root per sbaglio
	}

	// Sposta l'hitbox sul socket definito nel frame corrente del Flipbook
	// SnapToTargetNotIncludingScale mantiene la scala dell'hitbox indipendente dallo sprite (evita distorsioni)
	HitboxComponent->AttachToComponent(GetSprite(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, SocketName);
	
	HitboxComponent->SetBoxExtent(Extent);
	HitboxComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
}

void AGGJCharacter::DeactivateMeleeHitbox()
{
	HitboxComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	// Opzionale: Resetta la posizione se vuoi, ma non è strettamente necessario dato che viene riposizionata all'attivazione
}

void AGGJCharacter::PerformAttack()
{
	// If we are already attacking or doing something else, ignore input (or implement buffering here)
	if (ActionState != ECharacterActionState::None) return;
	
	PerformLunge(FindBestTarget(GetLastMovementInputVector()));
	
	// LOGIC:
	// If the Combo Timer is active, it means we are within the window of a previous attack -> Advance Combo.
	// If not, we are starting a new chain -> Reset to 0.
	if (GetWorld()->GetTimerManager().IsTimerActive(ComboTimerHandle))
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

	// Set state to Attacking (AnimBP will pick up AttackComboIndex and play the correct anim)
	ActionState = ECharacterActionState::Attacking;
	
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

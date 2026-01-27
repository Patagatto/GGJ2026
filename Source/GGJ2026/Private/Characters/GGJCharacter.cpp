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
#include "PaperFlipbook.h"
#include "Engine/DamageEvents.h"
#include "TimerManager.h"


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
	
	// Weapon Sprite: Visual representation of the weapon
	WeaponSprite = CreateDefaultSubobject<UPaperFlipbookComponent>(TEXT("WeaponSprite"));
	WeaponSprite->SetupAttachment(GetSprite()); // Initially attached to sprite root, will snap to socket later
	WeaponSprite->SetCollisionEnabled(ECollisionEnabled::NoCollision); // Purely visual

	// Hitbox: Deals damage
	HitboxComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("Hitbox"));
	HitboxComponent->SetupAttachment(RootComponent); // Attached to root, positioned via AnimNotifies or Logic
	HitboxComponent->SetBoxExtent(FVector(30.f, 30.f, 30.f));
	HitboxComponent->SetCollisionProfileName(TEXT("OverlapAllDynamic")); // Should be customized to only overlap Enemy Hurtboxes
	HitboxComponent->SetGenerateOverlapEvents(true);
	HitboxComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision); // Disabled by default! Enabled by Animation.

	// Bind the overlap event
	HitboxComponent->OnComponentBeginOverlap.AddDynamic(this, &AGGJCharacter::OnHitboxOverlap);
}

void AGGJCharacter::BeginPlay()
{
	Super::BeginPlay();
	
	// Unreal defaults to Falling on spawn. We force Walking so the first frame 
	// doesn't play the fall animation if we are spawning on the ground.
	// If we are actually in the air, the next physics update will correct it to Falling.
	GetCharacterMovement()->SetMovementMode(MOVE_Walking);

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
	ApplyMovementInput(MovementVector);
}

void AGGJCharacter::ApplyMovementInput(FVector2D MovementVector)
{
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
		// Apply Damage
		// UGameplayStatics::ApplyDamage(OtherActor, 10.0f, Controller, this, UDamageType::StaticClass());
	}
}

void AGGJCharacter::EquipWeapon(UPaperFlipbook* WeaponFlipbook, FName SocketName)
{
	if (WeaponSprite && GetSprite())
	{
		WeaponSprite->SetFlipbook(WeaponFlipbook);
		
		// Snap the weapon sprite to the specified socket on the main character sprite
		WeaponSprite->AttachToComponent(GetSprite(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, SocketName);
	}
}

void AGGJCharacter::UnequipWeapon()
{
	if (WeaponSprite)
	{
		WeaponSprite->SetFlipbook(nullptr);
	}
}

void AGGJCharacter::PerformAttack()
{
	// If we are already attacking or doing something else, ignore input (or implement buffering here)
	if (ActionState != ECharacterActionState::None) return;

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

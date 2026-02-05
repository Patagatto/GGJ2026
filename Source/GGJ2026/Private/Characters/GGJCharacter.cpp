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
#include "PaperSpriteComponent.h"
#include "GameFramework/DamageType.h"
#include "InputMappingContext.h"
#include "Game/GGJGamemode.h"


AGGJCharacter::AGGJCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#pragma region Components Setup
	// Capsule Setup
	GetCapsuleComponent()->InitCapsuleSize(30.f, 85.0f);
	GetCapsuleComponent()->SetUseCCD(true);

	// Rotation Setup: Controller rotates camera, not character
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

#pragma endregion

#pragma region Movement Setup
	// Movement Settings
	GetCharacterMovement()->GravityScale = 2.0f;
	GetCharacterMovement()->AirControl = 0.80f;
	GetCharacterMovement()->JumpZVelocity = 1000.f;
	GetCharacterMovement()->GroundFriction = 3.0f;
	GetCharacterMovement()->MaxWalkSpeed = 700.0f;
	GetCharacterMovement()->MaxFlySpeed = 600.0f;

	// HD-2D: Allow XY movement, disable planar constraint
	GetCharacterMovement()->bConstrainToPlane = false;
	
	// Disable capsule rotation; visual direction handled by Flipbook
	GetCharacterMovement()->bOrientRotationToMovement = false;
	GetCharacterMovement()->RotationRate = FRotator::ZeroRotator;

	// Prevent sliding off ledges
	GetCharacterMovement()->bUseFlatBaseForFloorChecks = true;
	
	// Default facing camera
	AnimDirection = 180.0f;
#pragma endregion

#pragma region Visuals & Combat Setup
	// Sprite Setup
	GetSprite()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetSprite()->SetCastShadow(true);

	// Sprite stays fixed to face camera (Billboard)
	GetSprite()->SetUsingAbsoluteRotation(true);
	GetSprite()->SetRelativeRotation(FRotator::ZeroRotator);
	// Scale Z to counteract camera foreshortening
	GetSprite()->SetRelativeScale3D(FVector(1.0f, 1.0f, 1.3f));

	// --- Combat Components Setup ---
	
	// Hurtbox Setup
	HurtboxComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("Hurtbox"));
	HurtboxComponent->SetupAttachment(GetSprite()); // Attach to sprite so it follows visual representation
	HurtboxComponent->SetBoxExtent(FVector(20.f, 10.f, 40.f)); // Made it thinner
	// Set the hurtbox to be of the "Pawn" object type
	HurtboxComponent->SetCollisionObjectType(ECC_GameTraceChannel6);
	// It should generate overlap events but not block anything by default
	HurtboxComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	HurtboxComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	HurtboxComponent->SetCollisionResponseToChannel(ECC_GameTraceChannel5, ECR_Overlap);
	HurtboxComponent->SetCollisionResponseToChannel(ECC_GameTraceChannel6, ECR_Overlap);
	HurtboxComponent->SetGenerateOverlapEvents(false);
	
	// Mask Sprite Setup
	MaskSprite = CreateDefaultSubobject<UPaperFlipbookComponent>(TEXT("MaskSprite"));
	MaskSprite->SetupAttachment(GetSprite());
	MaskSprite->SetCollisionEnabled(ECollisionEnabled::NoCollision); // Purely visual
	// Forward offset to prevent Z-fighting
	MaskSprite->SetRelativeLocation(FVector(0.0f, 1.0f, 0.0f));
	
	// Hitbox Setup
	HitboxComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("Hitbox"));
	HitboxComponent->SetupAttachment(GetSprite()); 
	HitboxComponent->SetBoxExtent(FVector(30.f, 30.f, 30.f));
	HitboxComponent->SetCollisionObjectType(ECC_GameTraceChannel3); // PlayerHitbox
	HitboxComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	HitboxComponent->SetCollisionResponseToChannel(ECC_GameTraceChannel4, ECR_Overlap); // EnemyHurtbox
	HitboxComponent->SetCollisionResponseToChannel(ECC_GameTraceChannel3, ECR_Overlap); // EnemyHurtbox
	HitboxComponent->SetGenerateOverlapEvents(false);
	HitboxComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision); // Disabled by default! Enabled by Animation.
	
	// Bind the overlap event
	HitboxComponent->OnComponentBeginOverlap.AddDynamic(this, &AGGJCharacter::OnHitboxOverlap);

	// Defaults
	ComboDamageValues = { 10.0f, 10.0f, 25.0f };

	MaxHealth = 100.0f;
	CurrentHealth = MaxHealth;

	InvincibilityDuration = 1.0f;
	HitStunDuration = 0.4f;
	KnockbackStrength = 600.0f;
	
	// Interaction Setup
	InteractionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionSphere"));
	InteractionSphere->SetupAttachment(RootComponent);
	InteractionSphere->SetSphereRadius(100.0f);
	InteractionSphere->SetCollisionProfileName(TEXT("Trigger"));
	InteractionSphere->OnComponentBeginOverlap.AddDynamic(this, &AGGJCharacter::OnInteractionSphereOverlapBegin);
	InteractionSphere->OnComponentEndOverlap.AddDynamic(this, &AGGJCharacter::OnInteractionSphereOverlapEnd);

	// Arrow Setup
	ArrowPivot = CreateDefaultSubobject<USceneComponent>(TEXT("ArrowPivot"));
	ArrowPivot->SetupAttachment(RootComponent);
	ArrowPivot->SetUsingAbsoluteRotation(true);
	ArrowPivot->SetRelativeLocation(FVector(0.0f, 0.0f, 2.0f)); 

	ArrowLineSprite = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("ArrowLineSprite"));
	ArrowLineSprite->SetupAttachment(ArrowPivot);
	ArrowLineSprite->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ArrowLineSprite->SetVisibility(true);
	ArrowLineSprite->SetRelativeRotation(FRotator(-90.0f, 0.0f, 0.0f));

	ArrowTipSprite = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("ArrowTipSprite"));
	ArrowTipSprite->SetupAttachment(ArrowPivot);
	ArrowTipSprite->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ArrowTipSprite->SetRelativeRotation(FRotator(-90.0f, 0.0f, 0.0f));
	ArrowTipSprite->SetVisibility(true);
	// Offset the tip so it sits at the end of the line (adjust 50.0f based on line length)
	ArrowTipSprite->SetRelativeLocation(FVector(50.0f, 0.0f, 0.0f));
	
#pragma endregion
}

void AGGJCharacter::BeginPlay()
{
	Super::BeginPlay();
	
	// Enforce absolute rotation for arrow pivot
	if (ArrowPivot)
	{
		ArrowPivot->SetUsingAbsoluteRotation(true);
	}
	
	// Force Walking state on spawn
	GetCharacterMovement()->SetMovementMode(MOVE_Walking);

	// Save defaults for restoration
	DefaultBrakingDeceleration = GetCharacterMovement()->BrakingDecelerationWalking;
	DefaultRollCooldown = RollCooldown;
	DefaultMaxWalkSpeed = GetCharacterMovement()->MaxWalkSpeed;
	
	// Center sprite relative to capsule
	GetSprite()->SetRelativeLocation(FVector(0.0f, 0.0f, GetSprite()->GetRelativeLocation().Z));

	// Initialize Health
	CurrentHealth = MaxHealth;

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

	// Initialize LastFacingDirection
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

	UpdateAnimationDirection();
	UpdateDirectionalArrow();
	
	// Reset input flag for the next frame
	bHasMovementInput = false;
	
	if (GetSprite()->DoesSocketExist("Hitbox"))
	{
		HitboxComponent->UpdateOverlaps();
	}
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
	Super::PawnClientRestart();

	// Apply Input Mapping Context
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
				UE_LOG(LogTemp, Error, TEXT("PawnClientRestart: DefaultMappingContext is NULL for player %d. Check the Character Blueprint!"), PlayerIndex);
			}
		}
	}
}

#pragma region Movement Logic

FRotator AGGJCharacter::GetCameraRotation() const
{
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		return PC->PlayerCameraManager ? PC->PlayerCameraManager->GetCameraRotation() : FRotator(-40.f, -90.f, 0.f);
	}
	// Fallback if no controller yet
	return FRotator(-40.f, -90.f, 0.f);
}

void AGGJCharacter::UpdateAnimationDirection()
{
	const FVector Velocity = GetVelocity();

	// Update LastFacingDirection from Velocity if moving
	if (Velocity.SizeSquared2D() > 1.0f)
	{
		LastFacingDirection = Velocity.GetSafeNormal2D();
	}
	
	const FRotator CameraRotation = GetCameraRotation();
	const FRotator TargetRotation = LastFacingDirection.ToOrientationRotator();

	// Calculate angle difference for AnimBP directional blend
	const float DeltaYaw = FRotator::NormalizeAxis(TargetRotation.Yaw - CameraRotation.Yaw);

	AnimDirection = DeltaYaw;

	// Perspective Correction for -40 degree camera
	const float PerspectiveCorrection = 1.3f;

	// Flip sprite based on direction
	if (DeltaYaw < -5.0f && DeltaYaw > -175.0f)
	{
		GetSprite()->SetRelativeScale3D(FVector(-1.0f, 1.0f, PerspectiveCorrection));
	}
	else
	{
		GetSprite()->SetRelativeScale3D(FVector(1.0f, 1.0f, PerspectiveCorrection));
	}
}

void AGGJCharacter::UpdateDirectionalArrow()
{
	// Arrow visibility logic commented out, keeping rotation update
	// const bool bShowArrow = bHasMovementInput || GetVelocity().SizeSquared2D() > 10.0f;
	
	FRotator TargetRotation = LastFacingDirection.Rotation();
	TargetRotation.Yaw += 90.0f;
	TargetRotation.Roll = 90.0f;
	TargetRotation.Pitch = 0.0f;
	
	const FRotator NewRotation = FMath::RInterpTo(ArrowPivot->GetComponentRotation(), TargetRotation, GetWorld()->GetDeltaSeconds(), 15.0f);
	
	ArrowPivot->SetWorldRotation(NewRotation);
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
		// Calculate movement direction based on Camera rotation
		const FRotator Rotation = GetCameraRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// Update LastFacingDirection based on Raw Input even if movement is blocked
		const FVector WorldInput = (ForwardDirection * MovementVector.Y) + (RightDirection * MovementVector.X);
		if (WorldInput.SizeSquared() > 0.01f)
		{
			LastFacingDirection = WorldInput.GetSafeNormal();
			bHasMovementInput = true;
		}

		// Block movement input during specific states
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
	if (OtherActor == this) return;
	
	if (HitActors.Contains(OtherActor)) return;

	// Check for Hurtbox tag
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

		// Apply Damage via GameplayStatics
		UGameplayStatics::ApplyDamage(OtherActor, DamageToDeal, GetController(), this, UDamageType::StaticClass());
		
		// Mask effects
		if (bExtendsDurationOnHit)
		{
			ExtendMaskDuration();
		}

		// If Red Rabbit mask is active (Lifesteal), heal the player
		if (bHasLifesteal)
		{
			CurrentHealth = FMath::Clamp(CurrentHealth + (DamageToDeal * RedRabbit_LifestealAmount), 0.0f, MaxHealth);
		}

		// Prevent multi-hit
		HitActors.Add(OtherActor);
	}
}

void AGGJCharacter::OnStunFinished()
{
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
		GetWorld()->GetTimerManager().SetTimer(InvincibilityOnGettingUpHandle, this, &AGGJCharacter::DisableInvincibility, InvincibilityTimeAfterKnock, false);
	}
}

void AGGJCharacter::DisableInvincibility()
{
	bIsInvincible = false;
	OnInvincibilityEnded();
}

void AGGJCharacter::InterruptCombatActions()
{
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

	// Clear timers
	GetWorld()->GetTimerManager().ClearTimer(HitCountResetTimerHandle);
	GetWorld()->GetTimerManager().ClearTimer(StunTimerHandle);
	GetWorld()->GetTimerManager().ClearTimer(InvincibilityTimerHandle);
	GetWorld()->GetTimerManager().ClearTimer(InvincibilityOnGettingUpHandle);

	// Invincibility during knockdown
	bIsInvincible = true;

	// Blueprint Event
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
		KnockbackDir = -LastFacingDirection.GetSafeNormal();
	}

	GetCharacterMovement()->StopMovementImmediately();

	// Add Z velocity to force 'Falling' state, ensuring 'Landed' event fires
	FVector LaunchVelocity = KnockbackDir.GetSafeNormal() * KnockdownPushStrength;
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
	else
	{
		// Fallback: Apply small knockback opposite to facing if damage source is unknown.
		LaunchCharacter(-LastFacingDirection.GetSafeNormal() * (KnockbackStrength * 0.5f), true, true);
	}

	// Set Stun Timer (When to regain control)
	GetWorld()->GetTimerManager().SetTimer(StunTimerHandle, this, &AGGJCharacter::OnStunFinished, HitStunDuration, false);

	// Temporary Invincibility
	bIsInvincible = true;
	GetWorld()->GetTimerManager().SetTimer(InvincibilityTimerHandle, this, &AGGJCharacter::DisableInvincibility, InvincibilityDuration, false);

	// Hit counter reset timer
	GetWorld()->GetTimerManager().ClearTimer(HitCountResetTimerHandle);
	GetWorld()->GetTimerManager().SetTimer(HitCountResetTimerHandle, this, &AGGJCharacter::ResetHitCount, HitCountResetTime, false);
}

void AGGJCharacter::HandleDeath()
{
	InterruptCombatActions();

	ActionState = ECharacterActionState::Dead;
	
	if (APlayerController* PC = Cast<APlayerController>(Controller))
	{
		DisableInput(PC);
	}

	OnPlayerDied();
	
	// Notify GameMode to check for Game Over
	if (AGGJGamemode* GM = Cast<AGGJGamemode>(UGameplayStatics::GetGameMode(this)))
	{
		GM->CheckPlayerStatus();
	}

	// Slow Motion Effect
	UGameplayStatics::SetGlobalTimeDilation(GetWorld(), 0.25f);

	// Reset Time Dilation
	GetWorld()->GetTimerManager().SetTimer(TimeDilationTimerHandle, this, &AGGJCharacter::ResetGlobalTimeDilation, 0.5f, false);
}

void AGGJCharacter::ResetGlobalTimeDilation()
{
	UGameplayStatics::SetGlobalTimeDilation(GetWorld(), 1.0f);
}

float AGGJCharacter::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

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

	if (ActualDamage <= 0.0f) return 0.0f;

	CurrentHealth = FMath::Clamp(CurrentHealth - ActualDamage, 0.0f, MaxHealth);

	if (CurrentHealth <= 0.0f)
	{
		HandleDeath();
		return ActualDamage;
	}

	InterruptCombatActions();

	CurrentHitCount++;
	
	// Determine if this hit causes a knockdown
	const bool bIsKnockdown = (CurrentHitCount >= HitsUntilKnockdown && !bIsImmuneToKnockdown);

	OnPlayerHit(bIsKnockdown);

	if (bIsKnockdown)
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
		// Reset combo on landing
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

	if (InputDirection.SizeSquared() > 0.01f)
	{
		SearchDirection = InputDirection.GetSafeNormal();
	}
	// Fallback to visual facing direction
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
	
	const FVector StartLoc = GetActorLocation();
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
	const FRotator CamRot = GetCameraRotation();
	FVector CameraRight = FRotationMatrix(CamRot).GetUnitAxis(EAxis::Y);
	CameraRight.Z = 0.0f;
	CameraRight.Normalize();

	// Determine if the player is currently on the left or right side of the target
	const FVector ActorLoc = GetActorLocation();
	const FVector VectorToPlayer = ActorLoc - Target->GetActorLocation();
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
	DestinationPoint.Z = ActorLoc.Z;

	// --- 2. Check if Lunge is Necessary ---
	const FVector VectorToDestination = DestinationPoint - ActorLoc;
	const float DistanceToDestination = VectorToDestination.Size2D();

	// Deadzone check
	if (DistanceToDestination <= 20.0f)
	{
		const FVector DirToActualTarget = (Target->GetActorLocation() - ActorLoc).GetSafeNormal2D();
		if (!DirToActualTarget.IsNearlyZero())
		{
			SetActorRotation(DirToActualTarget.Rotation());
		}
		return; // Do not lunge
	}

	// --- 3. Execute Lunge ---
	// Rotate to face the actual enemy, not the destination point
	const FVector DirToActualTarget = (Target->GetActorLocation() - ActorLoc).GetSafeNormal2D();
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
	if (!GetSprite()->DoesSocketExist(SocketName))
	{
		return;
	}

	HitboxComponent->AttachToComponent(GetSprite(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, SocketName);
	
	HitActors.Empty();

	HitboxComponent->SetBoxExtent(Extent);
	HitboxComponent->SetGenerateOverlapEvents(true);
	HitboxComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	HitboxComponent->UpdateOverlaps();
}

void AGGJCharacter::DeactivateMeleeHitbox()
{
	HitboxComponent->SetGenerateOverlapEvents(false);
	HitboxComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HitboxComponent->UpdateOverlaps();
}

void AGGJCharacter::ActivateMask(FName SocketName)
{
	if (!GetSprite()->DoesSocketExist(SocketName))
	{
		return;
	}

	MaskSprite->AttachToComponent(GetSprite(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, SocketName);

	MaskSprite->SetRelativeLocation(FVector(0.0f, 1.0f, 0.0f));
}

#pragma endregion

#pragma region Attack & Charge Logic

void AGGJCharacter::PerformAttack()
{
	if (ActionState != ECharacterActionState::None && ActionState != ECharacterActionState::Charging) return;
	
	// Aim Assist
	AActor* LungeTarget = FindBestTarget(GetLastMovementInputVector());
	PerformLunge(LungeTarget);
	
	// Combo Logic
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
	
	GetWorld()->GetTimerManager().ClearTimer(ComboTimerHandle);
	bPendingCombo = false;
	
	ActionState = ECharacterActionState::Attacking;

	// Blueprint Event
	const bool bHasMask = CurrentMaskType != EEnemyType::None;
	OnAttackStarted(false, bHasMask);
	
}

void AGGJCharacter::StartCharging()
{
	if (ActionState != ECharacterActionState::None) return;

	ActionState = ECharacterActionState::Charging;
	CurrentChargeTime = 0.0f;
	CurrentDamageMultiplier = 1.0f;
	
	// Stop movement immediately to prevent sliding while charging
	GetCharacterMovement()->StopMovementImmediately();

	OnChargeStarted();

	// Preserve combo state
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

}

void AGGJCharacter::FinishCharging()
{
	if (ActionState != ECharacterActionState::Charging) return;

	OnChargeEnded();

	// Calculate Damage Multiplier
	float Alpha = FMath::Clamp(CurrentChargeTime / MaxChargeTime, 0.0f, 1.0f);
	CurrentDamageMultiplier = FMath::Lerp(1.0f, MaxChargeDamageMultiplier, Alpha);

	// Execute
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
	if (ActionState == ECharacterActionState::Dead || 
		ActionState == ECharacterActionState::Hurt ||
		ActionState == ECharacterActionState::Grounded ||
		ActionState == ECharacterActionState::KnockedDown ||
		ActionState == ECharacterActionState::GettingUp ||
		ActionState == ECharacterActionState::Rolling ||
		bIsRollOnCooldown)
	{
		return;
	}

	// Interrupt combat actions
	if (ActionState == ECharacterActionState::Attacking || ActionState == ECharacterActionState::Charging)
	{
		InterruptCombatActions();
	}

	// Determine Direction
	FVector RollDirection = GetLastMovementInputVector();
	RollDirection.Z = 0.0f; // Force planar movement to prevent vertical drift
	
	// Handle small input / stationary roll
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

	// Sync facing direction
	LastFacingDirection = RollDirection;

	SetActorRotation(RollDirection.Rotation());

	// Snap arrow rotation
	if (ArrowPivot)
	{
		FRotator ArrowTarget = LastFacingDirection.Rotation();
		ArrowTarget.Yaw += 90.0f; // Same offset as UpdateDirectionalArrow
		ArrowTarget.Roll = 90.0f;
		ArrowTarget.Pitch = 0.0f;
		ArrowPivot->SetWorldRotation(ArrowTarget);
	}

	// Disable braking for smooth slide
	GetCharacterMovement()->BrakingDecelerationWalking = 0.0f;
	GetCharacterMovement()->GroundFriction = 2.0f; // Lower friction to slide farther
	// Override Z (true) to 0.0f to lock vertical movement and prevent hopping/diving on slopes
	LaunchCharacter(RollDirection * RollSpeed, true, true); 

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
	if (bInputConsumed) return;

	// Priority Check: Interact over Charge
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
	if (bInputConsumed)
	{
		bInputConsumed = false;
		return;
	}

	if (ActionState != ECharacterActionState::ChargeMask) return;

	// Reset State
	ActionState = ECharacterActionState::None;

	if (CurrentMaskType == EEnemyType::None) return;

	// Spawn mask projectile
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
		
		OnMaskLaunched();
		// Unequip without destroying the new actor
		UnequipMask(); 
	}
}

void AGGJCharacter::EquipMask(AMaskPickup* MaskToEquip)
{
	if (!MaskToEquip) return;

	if (CurrentMaskType != EEnemyType::None)
	{
		UnequipMask();
	}

	// Set new mask state
	CurrentMaskType = MaskToEquip->MaskType;
	CurrentMaskDuration = MaxMaskDuration;
	DrainRateMultiplier = 1.0f;
	OnMaskChanged(CurrentMaskType);

	ApplyBuff(CurrentMaskType);

	// Start the timer that drains the mask's duration
	GetWorld()->GetTimerManager().SetTimer(MaskDurationTimerHandle, this, &AGGJCharacter::UpdateMaskDuration, 1.0f, true, 1.0f);
	
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

	// Destroy pickup
	MaskToEquip->Destroy();
	OverlappingMask = nullptr;
}

void AGGJCharacter::UnequipMask()
{
	if (CurrentMaskType == EEnemyType::None) return;

	RemoveBuff(CurrentMaskType);

	CurrentMaskType = EEnemyType::None;
	CurrentMaskDuration = 0.0f;
	GetWorld()->GetTimerManager().ClearTimer(MaskDurationTimerHandle);
	MaskSprite->SetFlipbook(nullptr); // Hide the mask by removing its flipbook
	OnMaskChanged(CurrentMaskType);
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
	if (AMaskPickup* Mask = Cast<AMaskPickup>(OtherActor))
	{
		OverlappingMask = Mask;
		// TODO: Show "Interact" UI prompt
	}
}

void AGGJCharacter::OnInteractionSphereOverlapEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
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
		FTimerHandle StopJumpTimerHandle;
		GetWorld()->GetTimerManager().SetTimer(StopJumpTimerHandle, this, &ACharacter::StopJumping, 0.1f, false);
		bJumpStopPending = false;
	}
}

#pragma endregion

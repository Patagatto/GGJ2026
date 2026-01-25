// Fill out your copyright notice in the Description page of Project Settings.


#include "Characters/GGJCharacter.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "PaperFlipbookComponent.h"


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
	
	// FIX SLOWNESS: Changed from -20 to -45. At -20 the perspective flattened vertical movement making it seem slow.
	CameraBoom->SetRelativeRotation(FRotator(-45.0f, -90.0f, 0.0f)); 
	
	// Disable inheritance to prevent camera jitter during character rotation
	CameraBoom->bInheritPitch = false;
	CameraBoom->bInheritYaw = false;
	CameraBoom->bInheritRoll = false;
	CameraBoom->bDoCollisionTest = false; // Disable collision for 2D/Top-Down to prevent zooming in

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
	
	// FIX JITTER:
	// Disable physical capsule rotation. Visual direction is handled solely by animation.
	// This completely eliminates camera jitter.
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
}

void AGGJCharacter::BeginPlay()
{
	Super::BeginPlay();
	
	// Unreal defaults to Falling on spawn. We force Walking so the first frame 
	// doesn't play the fall animation if we are spawning on the ground.
	// If we are actually in the air, the next physics update will correct it to Falling.
	GetCharacterMovement()->SetMovementMode(MOVE_Walking);

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

		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
		// Move
		
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AGGJCharacter::Move);
	}
}

void AGGJCharacter::UpdateAnimationDirection()
{
	// FIX: Check only 2D velocity (XY). If falling vertically (Z), rotation would reset to 0.
	if (GetVelocity().SizeSquared2D() <= 1.0f) return;
	
	FRotator CameraRotation = FollowCamera->GetComponentRotation();
	FRotator VelocityRotation = GetVelocity().ToOrientationRotator();

	// Calculate the angle difference between the camera direction and velocity direction
	// This allows the AnimBP to select the correct directional animation (Front, Back, Side)
	float DeltaYaw = FRotator::NormalizeAxis(VelocityRotation.Yaw - CameraRotation.Yaw);

	AnimDirection = DeltaYaw;

	// FIX FLIP: If moving Left (negative DeltaYaw), flip the sprite.
	// DeltaYaw is approx -90 for left, +90 for right.
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

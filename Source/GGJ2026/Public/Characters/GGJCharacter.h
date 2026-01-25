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
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Animation)
	float AnimDirection;
	
	/** Speed of the character (XY plane magnitude) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Animation)
	float Speed;

	/** True if the character is moving faster than a small threshold */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Animation)
	bool bIsMoving;
	
	/** Jump state check booleans for Anim Blueprint*/
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Animation)
	bool bIsJumping;


protected:
	virtual void BeginPlay() override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	
	void UpdateAnimationDirection();
	
	
	/** Called for movement input */
	void Move(const FInputActionValue& Value);

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

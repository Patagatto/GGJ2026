// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MaskPickup.generated.h"

class UPaperFlipbookComponent;
class UBoxComponent;

/** Enum to define the different types of masks available. */
UENUM(BlueprintType)
enum class EMaskType : uint8
{
	None		UMETA(DisplayName = "None"),
	RedRabbit	UMETA(DisplayName = "Red Rabbit"),
	GreenBird	UMETA(DisplayName = "Green Bird"),
	BlueCat		UMETA(DisplayName = "Blue Cat")
};

UCLASS()
class GGJ2026_API AMaskPickup : public AActor
{
	GENERATED_BODY()
	
public:	
	AMaskPickup();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UPaperFlipbookComponent* Sprite;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UBoxComponent* InteractionVolume;

	/** The type of this mask, editable in the editor for each instance. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mask")
	EMaskType MaskType = EMaskType::RedRabbit;
};
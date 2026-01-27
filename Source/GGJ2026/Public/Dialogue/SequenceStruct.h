// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SequenceStruct.generated.h"

USTRUCT(BlueprintType)
struct GGJ2026_API FSequenceStruct
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere)
	bool IsText;
	
	UPROPERTY(EditAnywhere)
	TArray<FText> TextLines;
	
	// Create icons, for visual textbox
};

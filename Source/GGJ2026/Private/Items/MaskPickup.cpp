// Fill out your copyright notice in the Description page of Project Settings.


#include "Items/MaskPickup.h"
#include "PaperFlipbookComponent.h"
#include "Components/BoxComponent.h"

AMaskPickup::AMaskPickup()
{
	PrimaryActorTick.bCanEverTick = false;

	// Root component
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));

	// Sprite for the mask on the ground
	Sprite = CreateDefaultSubobject<UPaperFlipbookComponent>(TEXT("Sprite"));
	Sprite->SetupAttachment(RootComponent);
	Sprite->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// Volume to detect player interaction
	InteractionVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionVolume"));
	InteractionVolume->SetupAttachment(RootComponent);
	InteractionVolume->SetCollisionProfileName(TEXT("Trigger")); // "Trigger" profile overlaps with Pawns
	InteractionVolume->SetBoxExtent(FVector(50.f, 50.f, 50.f));
}
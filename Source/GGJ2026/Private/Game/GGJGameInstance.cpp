// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/GGJGameInstance.h"

void UGGJGameInstance::SetPlayMode(EPlayMode NewMode)
{
	PlayMode = NewMode;
}

EPlayMode UGGJGameInstance::GetPlayMode() const
{
	return PlayMode;
}

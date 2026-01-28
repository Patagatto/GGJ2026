// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/GGJGamemode.h"

AGGJGamemode::AGGJGamemode()
{
	TotalScore = 0;
}

void AGGJGamemode::AddScore(int32 Amount)
{
	TotalScore += Amount;
}

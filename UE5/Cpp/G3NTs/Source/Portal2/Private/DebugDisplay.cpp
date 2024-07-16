// Fill out your copyright notice in the Description page of Project Settings.


#include "DebugDisplay.h"

// Sets default values
ADebugDisplay::ADebugDisplay()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	bHasLineOfSightPortalOne = false;
	bCapturedSceneLastFrame = false;
	bCanTeleportNextUpdate = false;
	bHasTeleported = false;
	bTeleportTest = false;

	X = 0;
	Y = 0;
	Z = 0;

	rotX = 0;
	rotY = 0;
	rotZ = 0;

	PDot = 0;

	bTick = false;
}

// Called when the game starts or when spawned
void ADebugDisplay::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ADebugDisplay::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bTick == false)
	{
		return;
	}

	number++;
	if (number >= 99999)
	{
		number = 0;
	}

	if (bHasTeleported)
	{
		number2++;
	}
	if (number2 >= 100)
	{
		number2 = 0;
		bHasTeleported = false;
	}

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(1, 1.f, FColor::White, FString::Printf(TEXT("Tick Counter: %d"), number));
		GEngine->AddOnScreenDebugMessage(2, 1.f, FColor::White, FString::Printf(TEXT("Has Line of Sight: %s"), bHasLineOfSightPortalOne ? TEXT("true") : TEXT("false")));
		GEngine->AddOnScreenDebugMessage(3, 1.f, FColor::White, FString::Printf(TEXT("Captured Scene Last Frame: %s"), bCapturedSceneLastFrame ? TEXT("true") : TEXT("false")));
		GEngine->AddOnScreenDebugMessage(4, 1.f, FColor::White, FString::Printf(TEXT("Object Ready for Teleport: %s"), bCanTeleportNextUpdate ? TEXT("true") : TEXT("false")));
		GEngine->AddOnScreenDebugMessage(5, 1.f, FColor::White, FString::Printf(TEXT("Object Teleported: %s"), bHasTeleported ? TEXT("true") : TEXT("false")));
		GEngine->AddOnScreenDebugMessage(6, 1.f, FColor::White, FString::Printf(TEXT("Player inside collider box: %s"), bTeleportTest ? TEXT("true") : TEXT("false")));
		GEngine->AddOnScreenDebugMessage(8, 1.f, FColor::White, FString::Printf(TEXT("Portal plane dot: %f"), PDot));
	}
}


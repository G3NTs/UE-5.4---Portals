// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DebugDisplay.generated.h"

/**
 * This class is currently deprecated. Do not use!
 */
UCLASS()
class PORTAL2_API ADebugDisplay : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ADebugDisplay();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;


	int32 number = 0;
	int32 number2 = 0;
	bool bTick;
	bool bHasLineOfSightPortalOne;
	bool bCapturedSceneLastFrame;
	bool bCanTeleportNextUpdate;
	bool bHasTeleported;
	bool bTeleportTest;

	float X;
	float Y;
	float Z;

	float rotX;
	float rotY;
	float rotZ;

	float PDot;


};

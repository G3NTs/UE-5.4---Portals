// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "MyAnimInstance.generated.h"

/**
 * This animation instance is used for the player animation syncing. It contains floats and bools that are used by the animation blueprint to determine its animation state. These are synced through the PortalManager class.
 */
UCLASS()
class PORTAL2_API UMyAnimInstance : public UAnimInstance
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Animation")
    void SetAnimationPosition(float TimePosition);

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Animation")
    bool bIsMoving;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Animation")
    bool bIsInAir;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Animation")
    bool bHasRifle;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Animation")
    bool bTransitionUp;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Animation")
    bool bTransitionDown;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Animation")
    bool bOverrideBools;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Animation")
    bool bUpdateBools;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Animation")
    bool bFire;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Animation")
    float StartPosition;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Animation")
    float OutPosition;
};

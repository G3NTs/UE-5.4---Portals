// Fill out your copyright notice in the Description page of Project Settings.


#include "MyAnimInstance.h"
	
void UMyAnimInstance::SetAnimationPosition(float TimePosition)
{
    // Assuming you are dealing with a single animation sequence
   // Get the active montage or animation sequence
    UAnimMontage* CurrentMontage = GetCurrentActiveMontage();
    if (CurrentMontage)
    {
        Montage_SetPosition(CurrentMontage, TimePosition);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("--- No Montage ---"));
    }
}

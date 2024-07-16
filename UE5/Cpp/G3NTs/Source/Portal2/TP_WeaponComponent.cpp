// Copyright Epic Games, Inc. All Rights Reserved.

#include "TP_WeaponComponent.h"
#include "Portal2Character.h"
#include "Portal2Projectile.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "Kismet/GameplayStatics.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Animation/AnimInstance.h"
#include "GameFramework/Controller.h"
#include "Engine/LocalPlayer.h"
#include "MyAnimInstance.h"
#include "PortalBullet.h"
#include "Engine/World.h"

// Sets default values for this component's properties
UTP_WeaponComponent::UTP_WeaponComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PostUpdateWork;
	// Default offset from the character location for projectiles to spawn
	MuzzleOffset = FVector(25.0f, 10.0f, -10.0f);
}

void UTP_WeaponComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (Character == nullptr)
	{
		return;
	}

	if (APlayerController* PlayerController = Cast<APlayerController>(Character->GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->RemoveMappingContext(FireMappingContext);
		}
	}
}

void UTP_WeaponComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (bFireReady)
	{
		if (Timer >= 1)
		{
			// the fire animation is delayed one frame, so that the clone and main actor can start simultaniously.
			PlayFireAnimation(false);
		}
		Timer++;
	}
	else
	{
		Timer = 0;
	}
}

/**
 * Make the weapon Fire a Projectile
 */
void UTP_WeaponComponent::Fire()
{
	if (Character == nullptr || Character->GetController() == nullptr)
	{
		return;
	}

	// Try and fire a projectile
	if (ProjectileClass != nullptr)
	{
		UWorld* const World = GetWorld();
		if (World != nullptr)
		{
			APlayerController* PlayerController = Cast<APlayerController>(Character->GetController());
			const FRotator SpawnRotation = PlayerController->PlayerCameraManager->GetCameraRotation();
			// MuzzleOffset is in camera space, so transform it to world space before offsetting from the character location to find the final muzzle position
			const FVector SpawnLocation = PlayerController->PlayerCameraManager->GetCameraLocation() + SpawnRotation.RotateVector(MuzzleOffset);
	
			//Set Spawn Collision Handling Override
			FActorSpawnParameters ActorSpawnParams;
			ActorSpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding;
	
			// Spawn the projectile at the muzzle
			World->SpawnActor<APortal2Projectile>(ProjectileClass, SpawnLocation, SpawnRotation, ActorSpawnParams);
		}
	}
	
	// Try and play the sound if specified
	if (FireSound != nullptr)
	{
		UGameplayStatics::PlaySoundAtLocation(this, FireSound, Character->GetActorLocation());
	}

	PlayFireAnimation(true);
}

/**
 * Plays the fire animation
 *
 * @param bIn Boolean indicating whether to play the animation
 */
void UTP_WeaponComponent::PlayFireAnimation(bool bIn)
{
	// Try and play a firing animation if specified
	if (FireAnimation != nullptr)
	{
		// Get the animation object for the arms mesh
		UAnimInstance* AnimInstance = Character->GetMesh1P()->GetAnimInstance();
		if (AnimInstance != nullptr)
		{
			UMyAnimInstance* AnimInstanceCast = Cast<UMyAnimInstance>(AnimInstance);
			AnimInstanceCast->bFire = bIn;

			if (!bIn)
			{
				AnimInstance->Montage_Play(FireAnimation, 1.f);
				AnimInstanceCast->bFire = bIn;
				bFireReady = bIn;
			}
			else
			{
				bFireReady = bIn;
			}
		}
	}
}

/**
 * Fires a blue portal
 */
void UTP_WeaponComponent::FireBluePortal()
{
	FirePortal(false);
}

/**
 * Fires an orange portal
 */
void UTP_WeaponComponent::FireOrangePortal()
{
	FirePortal(true);
}

/**
 * Fires a portal based on the color specified
 *
 * @param bIsRed Boolean indicating whether to fire a red (orange) portal
 */
void UTP_WeaponComponent::FirePortal(bool bIsOrange)
{
	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	APlayerCameraManager* CameraManager = PlayerController->PlayerCameraManager;

	FVector SpawnLocation = CameraManager->GetCameraLocation() + CameraManager->GetCameraRotation().RotateVector(FVector(60.f,0.f,-10.f)); // Offset from the camera
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = GetOwner();

	AActor* PortalBullet;

	if (bIsOrange)
	{
		PortalBullet = GetWorld()->SpawnActor<AActor>(ABP_PortalBullet_Orange, SpawnLocation, CameraManager->GetCameraRotation(), SpawnParams);
		UE_LOG(LogTemp, Warning, TEXT("Orange"));
	}
	else
	{
		PortalBullet = GetWorld()->SpawnActor<AActor>(ABP_PortalBullet_Blue, SpawnLocation, CameraManager->GetCameraRotation(), SpawnParams);
		UE_LOG(LogTemp, Warning, TEXT("Blue"));
	}

	
	if (APortalBullet* PortalCast = Cast<APortalBullet>(PortalBullet))
	{
		PortalCast->bIsOrangePortal = bIsOrange;
	}

	if (FireSound != nullptr)
	{
		UGameplayStatics::PlaySoundAtLocation(this, FireSound, Character->GetActorLocation());
	}

	PlayFireAnimation(true);
}

/**
 * Changes the gun mode between portal mode and projectile mode
 */
void UTP_WeaponComponent::ChangeGunMode()
{
	CurrentGunMode = (CurrentGunMode == EGunMode::Projectile_Mode) ? EGunMode::Portal_Mode : EGunMode::Projectile_Mode;

	if (APlayerController* PlayerController = Cast<APlayerController>(Character->GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			// Set the priority of the mapping to 1, so that it overrides the Jump action with the Fire action when using touch input
			Subsystem->AddMappingContext(FireMappingContext, 1);
		}
		if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerController->InputComponent))
		{
			// Unbind all existing actions first
			EnhancedInputComponent->ClearActionBindings();

			// Bind actions based on the current gun mode
			if (CurrentGunMode == EGunMode::Projectile_Mode)
			{
				EnhancedInputComponent->BindAction(LeftFireAction, ETriggerEvent::Triggered, this, &UTP_WeaponComponent::Fire);
				EnhancedInputComponent->BindAction(GunModeAction, ETriggerEvent::Triggered, this, &UTP_WeaponComponent::ChangeGunMode);
			}
			else if (CurrentGunMode == EGunMode::Portal_Mode)
			{
				EnhancedInputComponent->BindAction(LeftFireAction, ETriggerEvent::Triggered, this, &UTP_WeaponComponent::FireBluePortal);
				EnhancedInputComponent->BindAction(RightFireAction, ETriggerEvent::Triggered, this, &UTP_WeaponComponent::FireOrangePortal);
				EnhancedInputComponent->BindAction(GunModeAction, ETriggerEvent::Triggered, this, &UTP_WeaponComponent::ChangeGunMode);
			}
		}
	}
}

/**
 * Attaches the weapon to a character
 *
 * @param TargetCharacter The character to attach the weapon to
 * @return True if the weapon was successfully attached, false otherwise
 */
bool UTP_WeaponComponent::AttachWeapon(APortal2Character* TargetCharacter)
{
	Character = TargetCharacter;

	// Check that the character is valid, and has no weapon component yet
	if (Character == nullptr || Character->GetInstanceComponents().FindItemByClass<UTP_WeaponComponent>())
	{
		return false;
	}

	// Attach the weapon to the First Person Character
	FAttachmentTransformRules AttachmentRules(EAttachmentRule::SnapToTarget, true);
	AttachToComponent(Character->GetMesh1P(), AttachmentRules, FName(TEXT("GripPoint")));

	// add the weapon as an instance component to the character
	Character->AddInstanceComponent(this);

	// Set up action bindings
	if (APlayerController* PlayerController = Cast<APlayerController>(Character->GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			// Set the priority of the mapping to 1, so that it overrides the Jump action with the Fire action when using touch input
			Subsystem->AddMappingContext(FireMappingContext, 1);
		}

		if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerController->InputComponent))
		{
			// Fire
			EnhancedInputComponent->BindAction(LeftFireAction, ETriggerEvent::Triggered, this, &UTP_WeaponComponent::Fire);
			EnhancedInputComponent->BindAction(GunModeAction, ETriggerEvent::Triggered, this, &UTP_WeaponComponent::ChangeGunMode);
		}
	}

	return true;
}
// Copyright Epic Games, Inc. All Rights Reserved.
// Modified version of the Epic Games weapon class.

#pragma once

#include "CoreMinimal.h"
#include "Components/SkeletalMeshComponent.h"
#include "TP_WeaponComponent.generated.h"

class APortal2Character;

/**
 * Enum to represent the different gun modes
 */
UENUM(BlueprintType)
enum class EGunMode : uint8
{
	Projectile_Mode,
	Portal_Mode
};

/**
 * UTP_WeaponComponent is a component that handles weapon functionalities such as firing projectiles or portals and changing gun modes.
 */
UCLASS(Blueprintable, BlueprintType, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class PORTAL2_API UTP_WeaponComponent : public USkeletalMeshComponent
{
	GENERATED_BODY()

public:
	/** Sets default values for this component's properties */
	UTP_WeaponComponent();

private:
	/** The Character holding this weapon*/
	APortal2Character* Character;

	/** The current gun mode */
	EGunMode CurrentGunMode;

	/** Boolean indicating whether the weapon is ready to fire */
	bool bFireReady;

	/** Timer for handling firing rate */
	int Timer = 0;

public:
	/** Projectile class to spawn */
	UPROPERTY(EditDefaultsOnly, Category=Projectile)
	TSubclassOf<class APortal2Projectile> ProjectileClass;

	/** Sound to play each time we fire */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Gameplay)
	USoundBase* FireSound;
	
	/** AnimMontage to play each time we fire */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	UAnimMontage* FireAnimation;

	/** Gun muzzle's offset from the characters location */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Gameplay)
	FVector MuzzleOffset;

	/** MappingContext */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess = "true"))
	class UInputMappingContext* FireMappingContext;

	/** Fire Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess = "true"))
	class UInputAction* LeftFireAction;

	/** Fire Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* RightFireAction;

	/** Fire Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* GunModeAction;

	/** Blue portal bullet class reference */
	UPROPERTY(EditAnywhere)
	TSubclassOf<AActor> ABP_PortalBullet_Blue;

	/** Orange portal bullet class reference */
	UPROPERTY(EditAnywhere)
	TSubclassOf<AActor> ABP_PortalBullet_Orange;

protected:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	/**
	 * Attaches the weapon to a character
	 *
	 * @param TargetCharacter The character to attach the weapon to
	 * @return True if the weapon was successfully attached, false otherwise
	 */
	UFUNCTION(BlueprintCallable, Category="Weapon")
	bool AttachWeapon(APortal2Character* TargetCharacter);

	/**
	 * Make the weapon Fire a Projectile
	 */
	UFUNCTION(BlueprintCallable, Category="Weapon")
	void Fire();

	/**
	 * Plays the fire animation
	 *
	 * @param bIn Boolean indicating whether to play the animation
	 */
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void PlayFireAnimation(bool bIn);

	/**
	 * Fires a blue portal
	 */
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void FireBluePortal();

	/**
	 * Fires an orange portal
	 */
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void FireOrangePortal();

	/**
	 * Fires a portal based on the color specified
	 *
	 * @param bIsRed Boolean indicating whether to fire a red (orange) portal
	 */
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void FirePortal(bool bIsRed);

	/**
	 * Changes the gun mode between portal mode and projectile mode
	 */
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void ChangeGunMode();

};

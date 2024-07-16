// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "Particles/ParticleSystemComponent.h"

#include "NiagaraComponent.h"

#include "GameFramework/PlayerController.h"

#include "PortalSurface.h"
#include "Portal3Manager.h"

#include "PortalBullet.generated.h"

class UProjectileMovementComponent;
class USphereComponent;
class APortal3Manager;

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class PORTAL2_API APortalBullet : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	APortalBullet();

private:
	UPROPERTY(EditDefaultsOnly, Category = "G3NTs|Portal")
	class USphereComponent* SphereCollision;

	UPROPERTY(EditDefaultsOnly, Category = "G3NTs|Portal")
	class UProjectileMovementComponent* ProjectileMovement;

	UPROPERTY(EditDefaultsOnly, Category = "G3NTs|Portal")
	class UNiagaraComponent* NiagaraComponent;

	UPROPERTY(EditDefaultsOnly, Category = "G3NTs|Portal")
	TSubclassOf<AActor> BP_Portal3Manager;

public:
	UPROPERTY(EditDefaultsOnly, Category = "G3NTs|Portal")
	bool bIsOrangePortal;
	
private:
	/**
	 * Called when the projectile hits something
	 *
	 * @param HitComp The component that was hit
	 * @param OtherActor The other actor involved in the collision
	 * @param OtherComp The other component involved in the collision
	 * @param NormalImpulse The force of the impact
	 * @param Hit Detailed information about the hit
	 */
	UFUNCTION()
	void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	/**
	 * Called when the projectile overlaps another object
	 *
	 * @param OverlappedComponent The component that was overlapped
	 * @param OtherActor The other actor involved in the overlap
	 * @param OtherComp The other component involved in the overlap
	 * @param OtherBodyIndex The index of the other body
	 * @param bFromSweep Whether the overlap was from a sweep
	 * @param SweepResult Detailed information about the sweep
	 */
	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	/**
	 * Finds the minimum and maximum vectors among the given points
	 *
	 * @param BR_Point Bottom-right point
	 * @param BL_Point Bottom-left point
	 * @param TR_Point Top-right point
	 * @param TL_Point Top-left point
	 * @return A pair containing the minimum and maximum vectors
	 */
	std::pair<FVector, FVector> FindMinMax(const FVector& BR_Point, const FVector& BL_Point, const FVector& TR_Point, const FVector& TL_Point);

	/**
	 * Adjusts the portal's position on the surface
	 *
	 * @param PortalSurfaceData The portal surface data component
	 * @param LocalPortalMin The local minimum vector of the portal
	 * @param LocalPortalMax The local maximum vector of the portal
	 * @param PortalCenter The center point of the portal
	 * @param SurfaceOrigin The origin of the surface
	 * @param SurfaceExtend The extent of the surface
	 * @param Index The index of the portal
	 * @param Displacement The displacement vector
	 * @param Rotation The rotation of the portal
	 * @return true if the portal's position is adjusted, false otherwise
	 */
	bool AdjustPortalPosition(UPortalSurface* PortalSurfaceData, FVector& LocalPortalMin, FVector& LocalPortalMax, FVector& PortalCenter, const FVector& SurfaceOrigin, const FVector& SurfaceExtend, int32& Index, FVector& Displacement, FRotator& Rotation);

	/**
	 * Spawns the portal on the surface
	 *
	 * @param PortalCenter The center point of the portal
	 * @param PortalRotation The rotation of the portal
	 * @param SurfaceForwardVector The forward vector of the surface
	 * @param PortalSurfaceData The portal surface data component
	 * @param Index The index of the portal
	 */
	void SpawnPortalOnSurface(FVector PortalCenter, FQuat PortalRotation, FVector SurfaceForwardVector, UPortalSurface* PortalSurfaceData, int32 Index);

};

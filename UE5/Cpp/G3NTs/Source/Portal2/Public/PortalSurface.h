// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Components/DynamicMeshComponent.h"
#include "DynamicMeshBuilder.h"
#include "UDynamicMesh.h"
#include "Components/StaticMeshComponent.h"
#include "PortalSurface.generated.h"

class APortal3Manager;

/**
 * Event called to send a portal data set into the blueprint. 
 * 
 * It takes three parameters:
 * - int32 - Key of the portal item
 * - FVector - Vector storing the center point of the portal actor.
 * - FRotator - FRotator storing the rotation of the portal actor.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnProcessMapItem,
	int32, key,
	FVector, Center,
	FRotator, Rotation
);

/**
 * Delegate to fire a event in blueprints to rebuild collisions on the surface mesh. 
 * Delegates require an empty class assignment, 
 * this functions as a handle which can be used to fire the event.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FRebuildCollision);

/**
 * Structure to combine variables into a single value class for a TMap.
 */
USTRUCT(BlueprintType)
struct FPortalData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Portals")
	FVector Min;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Portals")
	FVector Max;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Portals")
	FVector Center;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Portals")
	FRotator Rotation;

	FPortalData() : Min(FVector::ZeroVector), Max(FVector::ZeroVector), Center(FVector::ZeroVector), Rotation(FRotator::ZeroRotator) {}

	FPortalData(const FVector& InMin, const FVector& InMax, const FVector& InCenter, const FRotator& InRotation)
		: Min(InMin), Max(InMax), Center(InCenter), Rotation(InRotation) {}
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class PORTAL2_API UPortalSurface : public UActorComponent
{
	GENERATED_BODY()

	int32 UniquePortalId;

public:	
	// Sets default values for this component's properties
	UPortalSurface();

protected:
	void BeginPlay() override;

private:
	UPROPERTY(BlueprintAssignable)
	FOnProcessMapItem OnProcessMapItem; // Class which a delegate is assigned to. 

	UPROPERTY(BlueprintAssignable)
	FRebuildCollision RebuildCollision; // Class which a delegate is assigned to. 

	UPROPERTY(EditAnywhere, Category = "Portals")
	TSubclassOf<AActor> ManagerClass; // Reference to the portal manager class.

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Portals")
	TMap<int32, FPortalData> Portals; // TMap to store portals which are attachted to this surface.

public:
	UDynamicMeshComponent* DynamicMeshComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Portals")
	UDynamicMesh* Mesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Portals")
	FVector InlineScale = FVector(1, 1, 1); // A inline scale value, used to rescale the dynamic mesh. As using the actual scale parameters breaks the portal placement algorithm.

private:
	/**
	 * Sets the dynamic mesh
	 *
	 * @param InMesh The dynamic mesh to set
	 */
	UFUNCTION(BlueprintCallable, Category = "Portals")
	void SetMesh(UDynamicMesh* InMesh);

	/**
	 * Gets the dynamic mesh
	 *
	 * @return The dynamic mesh
	 */
	UFUNCTION(BlueprintCallable, Category = "Portals")
	UDynamicMesh* GetMesh() const;

	/**
	 * Sets the scale of the inline surface mesh, without changing the actual scale of the actor.
	 *
	 * @param Scale The scale to set
	 */
	UFUNCTION(BlueprintCallable, Category = "Portals")
	void SetInlineScale(FVector Scale);

	/**
	 * Gets the scale of the inline surface mesh.
	 *
	 * @return The scale of the inline mesh
	 */
	UFUNCTION(BlueprintCallable, Category = "Portals")
	FVector GetInlineScale() const;

	/**
	 * Iterates over the portal TMap and processes each item
	 */
	UFUNCTION(BlueprintCallable, Category = "Portals")
	void IterateMap();

	/**
	 * Sets the visibility of the dynamic mesh.
	 *
	 * @param bVisible Whether the mesh should be visible
	 */
	void SetMeshVisibility(bool bVisible);

public:
	/**
	 * Rebuilds the collision mesh
	 */
	void RebuildCollisionMesh();

	/**
	 * Adds a portal to the TMap.
	 *
	 * @param Min The minimum vector of the portal domain
	 * @param Max The maximum vector of the portal domain
	 * @param Center The center point of the portal
	 * @param Rotation The rotation of the portal
	 * @return The ID of the added portal
	 */
	int32 AddPortal(const FVector& Min, const FVector& Max, const FVector& Center, const FRotator& Rotation);

	/**
	 * Gets a portal from the TMap.
	 *
	 * @param PortalID The ID of the portal to get
	 * @param Min The minimum vector of the portal domain (output)
	 * @param Max The maximum vector of the portal domain (output)
	 * @param Center The center point of the portal (output)
	 * @return true if the portal is found, false otherwise
	 */
	bool GetPortal(int32 PortalID, FVector& Min, FVector& Max, FVector& Center) const;

	/**
	 * Updates a portal in the TMap.
	 *
	 * @param PortalID The ID of the portal to update
	 * @param Min The new minimum vector of the portal domain
	 * @param Max The new maximum vector of the portal domain
	 * @param Center The new center point of the portal
	 * @return true if the portal is updated, false otherwise
	 */
	bool UpdatePortal(int32 PortalID, FVector Min, FVector Max, FVector Center);

	/**
	 * Removes a portal from the TMap.
	 *
	 * @param PortalID The ID of the portal to remove
	 * @return true if the portal is removed, false otherwise
	 */
	bool RemovePortal(int32 PortalID);

	/**
	 * Moves a portal if it overlaps with another portal.
	 *
	 * @param PortalID The ID of the portal to move
	 * @return true if the portal is moved, false otherwise
	 */
	bool MovePortalOnOverlap(int32 PortalID);

	/**
	 * Fits a portal to the surface.
	 *
	 * @param PortalID The ID of the portal to fit
	 * @param BoxMin The minimum vector of the bounding box
	 * @param BoxMax The maximum vector of the bounding box
	 */
	void FitPortalToSurface(int32 PortalID, const FVector BoxMin, const FVector BoxMax);
};

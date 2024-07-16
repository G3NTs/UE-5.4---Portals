// Fill out your copyright notice in the Description page of Project Settings.


#include "PortalSurface.h"
#include "Portal3Manager.h"
#include "DynamicMeshBuilder.h"
#include "Components/StaticMeshComponent.h"

// Sets default values for this component's properties
UPortalSurface::UPortalSurface() : UniquePortalId(0)
{
	PrimaryComponentTick.bCanEverTick = false;
	Mesh = nullptr;
}

void UPortalSurface::BeginPlay()
{
	Super::BeginPlay();

	AActor* Owner = GetOwner();
	if (Owner)
	{
		DynamicMeshComponent = Owner->FindComponentByClass<UDynamicMeshComponent>();
	}
}

/**
 * Sets the scale of the inline surface mesh, without changing the actual scale of the actor.
 *
 * @param Scale The scale to set
 */
void UPortalSurface::SetInlineScale(FVector Scale)
{
	InlineScale = Scale;
}

/**
 * Gets the scale of the inline surface mesh.
 *
 * @return The scale of the inline mesh
 */
FVector UPortalSurface::GetInlineScale() const
{
	return InlineScale;
}

/**
 * Rebuilds the collision mesh
 */
void UPortalSurface::RebuildCollisionMesh()
{
	RebuildCollision.Broadcast();
}

/**
 * Sets the visibility of the dynamic mesh.
 *
 * @param bVisible Whether the mesh should be visible
 */
void UPortalSurface::SetMeshVisibility(bool bVisible)
{
	if (DynamicMeshComponent)
	{
		DynamicMeshComponent->SetVisibility(bVisible, false);
		DynamicMeshComponent->SetHiddenInGame(!bVisible, false);
	}
}

/**
 * Adds a portal to the TMap.
 *
 * @param Min The minimum vector of the portal domain
 * @param Max The maximum vector of the portal domain
 * @param Center The center point of the portal
 * @param Rotation The rotation of the portal
 * @return The ID of the added portal
 */
int32 UPortalSurface::AddPortal(const FVector& Min, const FVector& Max, const FVector& Center, const FRotator& Rotation)
{
	int32 Index = ++UniquePortalId;
	UE_LOG(LogTemp, Warning, TEXT("Adding Portal to SurfaceMap, Index: %d"), Index);
	Portals.Add(Index, FPortalData(Min, Max, Center, Rotation));
	return Index;
}

/**
 * Gets a portal from the TMap.
 *
 * @param PortalID The ID of the portal to get
 * @param Min The minimum vector of the portal domain (output)
 * @param Max The maximum vector of the portal domain (output)
 * @param Center The center point of the portal (output)
 * @return true if the portal is found, false otherwise
 */
bool UPortalSurface::GetPortal(int32 PortalID, FVector& Min, FVector& Max, FVector& Center) const
{
	UE_LOG(LogTemp, Warning, TEXT("Getting Portal from SurfaceMap, Index: %d"), PortalID);
	const FPortalData* PortalData = Portals.Find(PortalID);
	if (PortalData)
	{
		Min = PortalData->Min;
		Max = PortalData->Max;
		Center = PortalData->Center;

		return true;
	}
	return false;
}

/**
 * Updates a portal in the TMap.
 *
 * @param PortalID The ID of the portal to update
 * @param Min The new minimum vector of the portal domain
 * @param Max The new maximum vector of the portal domain
 * @param Center The new center point of the portal
 * @return true if the portal is updated, false otherwise
 */
bool UPortalSurface::UpdatePortal(int32 PortalID, FVector Min, FVector Max, FVector Center)
{
	UE_LOG(LogTemp, Warning, TEXT("Updating Portal from SurfaceMap, Index: PortalID"), PortalID);
	FPortalData* PortalData = Portals.Find(PortalID);
	if (PortalData)
	{
		PortalData->Min = Min;
		PortalData->Max = Max;
		PortalData->Center = Center;
		return true;
	}
	return false;
}

/**
 * Removes a portal from the TMap.
 *
 * @param PortalID The ID of the portal to remove
 * @return true if the portal is removed, false otherwise
 */
bool UPortalSurface::RemovePortal(int32 PortalID)
{
	UE_LOG(LogTemp, Warning, TEXT("Removing Portal From SurfaceMap, Index: %d"), PortalID);
	Portals.Remove(PortalID);
	RebuildCollisionMesh();
	return true;
}

/**
 * Moves a portal if it overlaps with another portal.
 *
 * @param PortalID The ID of the portal to move
 * @return true if the portal is moved, false otherwise
 */
bool UPortalSurface::MovePortalOnOverlap(int32 PortalID)
{
	bool bMoved = false;

	FVector Min, Max, Center;
	GetPortal(PortalID, Min, Max, Center);

	for (const auto& Elem : Portals)
	{
		int32 OtherPortalID = Elem.Key;

		if (PortalID == OtherPortalID)
		{
			continue; 
		}

		const FPortalData* OtherPortal = Portals.Find(OtherPortalID);

		bool OverlappingX = (Max.X > OtherPortal->Min.X && Min.X < OtherPortal->Max.X);
		bool OverlappingY = (Max.Y > OtherPortal->Min.Y && Min.Y < OtherPortal->Max.Y);

		if (OverlappingX && OverlappingY)
		{
			float MoveX = 0.0f;
			float MoveY = 0.0f;

			DrawDebugSphere(GetWorld(), Min + FVector(0,0,1400), 10.0f, 12, FColor::Blue, false, 300.0f);
			DrawDebugSphere(GetWorld(), Max + FVector(0, 0, 1400), 10.0f, 12, FColor::Cyan, false, 300.0f);
			DrawDebugSphere(GetWorld(), OtherPortal->Min + FVector(0, 0, 1400), 10.0f, 12, FColor::Red, false, 300.0f);
			DrawDebugSphere(GetWorld(), OtherPortal->Max + FVector(0, 0, 1400), 10.0f, 12, FColor::Orange, false, 300.0f);

			MoveX = (FMath::Abs(OtherPortal->Min.X - Max.X) < FMath::Abs(OtherPortal->Max.X - Min.X)) ? (OtherPortal->Min.X - Max.X) : (OtherPortal->Max.X - Min.X);
			MoveY = (FMath::Abs(OtherPortal->Min.Y - Max.Y) < FMath::Abs(OtherPortal->Max.Y - Min.Y)) ? (OtherPortal->Min.Y - Max.Y) : (OtherPortal->Max.Y - Min.Y);

			UE_LOG(LogTemp, Warning, TEXT("Center1 variable is: %s"), *Center.ToString());
			UE_LOG(LogTemp, Warning, TEXT("Center2 variable is: %s"), *OtherPortal->Center.ToString());
			UE_LOG(LogTemp, Warning, TEXT("================="));
			UE_LOG(LogTemp, Warning, TEXT("Min variable is: %s"), *Min.ToString());
			UE_LOG(LogTemp, Warning, TEXT("Max variable is: %s"), *Max.ToString());
			UE_LOG(LogTemp, Warning, TEXT("-----------------"));
			UE_LOG(LogTemp, Warning, TEXT("Min variable is: %s"), *OtherPortal->Min.ToString());
			UE_LOG(LogTemp, Warning, TEXT("Max variable is: %s"), *OtherPortal->Max.ToString());
			UE_LOG(LogTemp, Warning, TEXT("================="));
			UE_LOG(LogTemp, Warning, TEXT("MoveX variable is: %f"), MoveX);
			UE_LOG(LogTemp, Warning, TEXT("MoveY variable is: %f"), MoveY);

			if (FMath::Abs(MoveX) < FMath::Abs(MoveY))
			{
				Min.X += MoveX;
				Max.X += MoveX;
				Center.X += MoveX;
			}
			else
			{
				Min.Y += MoveY;
				Max.Y += MoveY;
				Center.Y += MoveY;
			}

			bMoved = true;
		}
	}

	UpdatePortal(PortalID, Min, Max, Center);

	return !bMoved;
}

/**
 * Fits a portal to the surface.
 *
 * @param PortalID The ID of the portal to fit
 * @param BoxMin The minimum vector of the bounding box
 * @param BoxMax The maximum vector of the bounding box
 */
void UPortalSurface::FitPortalToSurface(int32 PortalID, const FVector BoxMin, const FVector BoxMax)
{
	FVector Min, Max, Center;
	GetPortal(PortalID, Min, Max, Center);

	//UE_LOG(LogTemp, Warning, TEXT("LocalHitLocation before adjustment: %s"), *Center.ToString())

	if (Min.X < BoxMin.X) {
		float Delta = BoxMin.X - Min.X;
		Min.X += Delta;
		Max.X += Delta;
		Center.X += Delta;
	}
	if (Max.X > BoxMax.X) {
		float Delta = Max.X - BoxMax.X;
		Min.X -= Delta;
		Max.X -= Delta;
		Center.X -= Delta;
	}
	if (Min.Y < BoxMin.Y) {
		float Delta = BoxMin.Y - Min.Y;
		Min.Y += Delta;
		Max.Y += Delta;
		Center.Y += Delta;
	}
	if (Max.Y > BoxMax.Y) {
		float Delta = Max.Y - BoxMax.Y;
		Min.Y -= Delta;
		Max.Y -= Delta;
		Center.Y -= Delta;
	}

	UpdatePortal(PortalID, Min, Max, Center);

	//UE_LOG(LogTemp, Warning, TEXT("LocalHitLocation before adjustment: %s"), *Center.ToString())
}

/**
 * Iterates over the portal TMap and processes each item
 */
void UPortalSurface::IterateMap()
{
	TArray<AActor*> Managers;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ManagerClass, Managers);
	APortal3Manager* PortalManager = Cast<APortal3Manager>(Managers[0]);
	if (PortalManager->OrangePortal && PortalManager->BluePortal)
	{
		// Iterate over the map
		for (const auto& Elem : Portals)
		{
			int32 Key = Elem.Key;
			const FPortalData PortalValue = Elem.Value;

			DrawDebugSphere(GetWorld(), PortalValue.Center + GetOwner()->GetActorLocation(), 30.0f, 12, FColor::Black, false, 5.0f);

			OnProcessMapItem.Broadcast(Key, PortalValue.Center, PortalValue.Rotation);
		}
	}
}

/**
 * Sets the dynamic mesh
 *
 * @param InMesh The dynamic mesh to set
 */
void UPortalSurface::SetMesh(UDynamicMesh* InMesh)
{
	Mesh = InMesh;
}

/**
 * Gets the dynamic mesh
 *
 * @return The dynamic mesh
 */
UDynamicMesh* UPortalSurface::GetMesh() const
{
	return Mesh;
}
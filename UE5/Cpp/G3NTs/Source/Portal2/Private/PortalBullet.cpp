// Fill out your copyright notice in the Description page of Project Settings.

#include "PortalBullet.h"

#include "GameFramework/ProjectileMovementComponent.h"
#include "Components/SphereComponent.h"
#include "Kismet/KismetMathLibrary.h"

// Sets default values
APortalBullet::APortalBullet()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	SphereCollision = CreateDefaultSubobject<USphereComponent>(TEXT("SphereCollision"));
	SphereCollision->InitSphereRadius(15.0f);  // Set the radius as desired

	SphereCollision->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	SphereCollision->SetCollisionObjectType(ECC_WorldDynamic);
	SphereCollision->SetCollisionResponseToAllChannels(ECR_Block);
	SphereCollision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);

	SphereCollision->OnComponentBeginOverlap.AddDynamic(this, &APortalBullet::OnOverlapBegin);
	SphereCollision->OnComponentHit.AddDynamic(this, &APortalBullet::OnHit);

	SphereCollision->SetWalkableSlopeOverride(FWalkableSlopeOverride(WalkableSlope_Unwalkable, 0.f));
	SphereCollision->CanCharacterStepUpOn = ECB_No;

	RootComponent = SphereCollision;

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->UpdatedComponent = SphereCollision;
	ProjectileMovement->InitialSpeed = 2000.f;
	ProjectileMovement->MaxSpeed = 2000.f;
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->bShouldBounce = false;

	NiagaraComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("NiagaraComponent"));
	NiagaraComponent->SetupAttachment(SphereCollision);

	InitialLifeSpan = 5.0f;
}

/**
 * Called when the projectile hits something
 *
 * @param HitComp The component that was hit
 * @param OtherActor The other actor involved in the collision
 * @param OtherComp The other component involved in the collision
 * @param NormalImpulse The force of the impact
 * @param Hit Detailed information about the hit
 */
void APortalBullet::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (OtherActor && OtherActor != this && OtherComp)
	{
		// Check if the hit component is of PortalSurface channel
		if (OtherComp->GetCollisionObjectType() == ECC_GameTraceChannel2) // Ensure this is set to PortalSurface channel
		{
			TArray<AActor*> ManagerArray;
			if (BP_Portal3Manager != nullptr)
			{
				UGameplayStatics::GetAllActorsOfClass(GetWorld(), BP_Portal3Manager, ManagerArray);
				APortal3Manager* PortalManager = Cast<APortal3Manager>(ManagerArray[0]);
				PortalManager->DestroyOldPortal(bIsOrangePortal);
			}

			UE_LOG(LogTemp, Warning, TEXT("================= Portal surface hit! ================="));
			UStaticMeshComponent* Plane = OtherActor->GetComponentByClass<UStaticMeshComponent>();
			UPortalSurface* PortalSurfaceData = OtherActor->GetComponentByClass<UPortalSurface>();

			if (PortalSurfaceData)
			{
				UE_LOG(LogTemp, Warning, TEXT("No Portal Surface?!?"));
			}

			FVector SurfaceOrigin, SurfaceExtend;
			Plane->GetLocalBounds(SurfaceOrigin, SurfaceExtend);

			SurfaceOrigin = Plane->GetOwner()->GetActorLocation() + Plane->GetOwner()->GetActorRotation().RotateVector(Plane->GetRelativeLocation() * Plane->GetRelativeScale3D());
			SurfaceExtend *= Plane->GetRelativeScale3D();//GetOwner()->GetActorScale3D();

			DrawDebugBox(GetWorld(), FVector(0, 0, 1400), SurfaceExtend, FRotator(0, 0, 0).Quaternion(), FColor::Green, false, 5.0f, 0, 2.0f);
			DrawDebugSphere(GetWorld(), Hit.ImpactPoint, 10.0f, 12, FColor::Red, false, 5.0f);

			FVector LocalHitLocation = Plane->GetComponentTransform().InverseTransformPosition(Hit.ImpactPoint);
			//LocalHitLocation.Z += 1400;
			LocalHitLocation *= Plane->GetRelativeScale3D();

			DrawDebugSphere(GetWorld(), LocalHitLocation, 10.0f, 12, FColor::Red, false, 5.0f);

			FVector PortalSize(120.f, 240.f, 0.f);

			FVector LocalPortalMin = LocalHitLocation - (PortalSize / 2.0f);
			FVector LocalPortalMax = LocalHitLocation + (PortalSize / 2.0f);

			FVector PortalCenter = Hit.ImpactPoint;
			FVector PortalExtend = PortalSize / 2.0f;
			FQuat BoxRotation = OtherActor->GetActorRotation().Quaternion(); // Assuming the portal aligns with the plane's rotation

			APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
			if (PlayerController)
			{
				APawn* PlayerPawn = PlayerController->GetPawn();
				if (PlayerPawn)
				{
					FVector SurfaceForward = Plane->GetOwner()->GetActorForwardVector();
					FVector SurfaceRight = Plane->GetOwner()->GetActorRightVector();
					FVector SurfaceUp = Plane->GetOwner()->GetActorUpVector();
					FVector PlayerRight = PlayerPawn->GetActorRightVector();

					// Calculate the roll angle
					float DotForward = FVector::DotProduct(SurfaceForward, PlayerRight);
					float DotRight = FVector::DotProduct(SurfaceRight, PlayerRight);

					FVector PortalUpVector = FVector(SurfaceForward * DotForward + SurfaceRight * DotRight).GetSafeNormal();
					FVector PortalForwardVector = SurfaceUp.GetSafeNormal();
					FVector PortalRightVector = FVector::CrossProduct(PortalUpVector, PortalForwardVector).GetSafeNormal();

					DrawDebugDirectionalArrow(GetWorld(), PortalCenter, PortalCenter + PortalUpVector * 100, 20.f, FColor::Blue, false, 5.0f, 0, 2.0f);
					DrawDebugDirectionalArrow(GetWorld(), PortalCenter, PortalCenter + SurfaceRight * 100, 20.f, FColor::Orange, false, 5.0f, 0, 2.0f);
					DrawDebugDirectionalArrow(GetWorld(), PortalCenter, PortalCenter + SurfaceForward * 100, 20.f, FColor::Orange, false, 5.0f, 0, 2.0f);

					FRotator BoxRotation2 = UKismetMathLibrary::MakeRotationFromAxes(PortalUpVector, PortalRightVector, -PortalForwardVector);

					FQuat BoxRotation3 = Plane->GetOwner()->GetActorRotation().Quaternion().Inverse() * BoxRotation2.Quaternion();

					FVector BR_Point, BL_Point, TR_Point, TL_Point;

					BR_Point = FVector(PortalExtend.X, -PortalExtend.Y, 0);
					BL_Point = FVector(-PortalExtend.X, -PortalExtend.Y, 0);
					TR_Point = FVector(PortalExtend.X, PortalExtend.Y, 0);
					TL_Point = FVector(-PortalExtend.X, PortalExtend.Y, 0);

					BR_Point = BoxRotation3.RotateVector(BR_Point) + LocalHitLocation;
					BL_Point = BoxRotation3.RotateVector(BL_Point) + LocalHitLocation;
					TR_Point = BoxRotation3.RotateVector(TR_Point) + LocalHitLocation;
					TL_Point = BoxRotation3.RotateVector(TL_Point) + LocalHitLocation;

					DrawDebugSphere(GetWorld(), FVector(BR_Point.X, BR_Point.Y, 1400), 10.0f, 12, FColor::Black, false, 5.0f);
					DrawDebugSphere(GetWorld(), FVector(BL_Point.X, BL_Point.Y, 1400), 10.0f, 12, FColor::Black, false, 5.0f);
					DrawDebugSphere(GetWorld(), FVector(TR_Point.X, TR_Point.Y, 1400), 10.0f, 12, FColor::Black, false, 5.0f);
					DrawDebugSphere(GetWorld(), FVector(TL_Point.X, TL_Point.Y, 1400), 10.0f, 12, FColor::Black, false, 5.0f);

					std::pair<FVector, FVector> MinMaxVectors = FindMinMax(BR_Point, BL_Point, TR_Point, TL_Point);

					LocalPortalMin = MinMaxVectors.first;
					LocalPortalMax = MinMaxVectors.second;

					FVector NewBoxExtend((LocalPortalMax.X - LocalPortalMin.X) / 2, (LocalPortalMax.Y - LocalPortalMin.Y) / 2, 0);
					int32 Index;
					FVector Displacement;
					FRotator BoxRotation4 = BoxRotation3.Rotator();

					if (!AdjustPortalPosition(PortalSurfaceData, LocalPortalMin, LocalPortalMax, LocalHitLocation, FVector(0, 0, 0), SurfaceExtend, Index, Displacement, BoxRotation4))
					{
						PortalSurfaceData->RemovePortal(Index);
						Destroy();
						return;
					}

					Displacement = BoxRotation.RotateVector(Displacement);
					PortalCenter += Displacement;

					bool bCanFit = (LocalPortalMin.X >= -SurfaceExtend.X && LocalPortalMax.X <= SurfaceExtend.X) && (LocalPortalMin.Y >= -SurfaceExtend.Y && LocalPortalMax.Y <= SurfaceExtend.Y);

					if (bCanFit)
					{
						SpawnPortalOnSurface(PortalCenter, BoxRotation2.Quaternion(), OtherActor->GetActorUpVector(), PortalSurfaceData, Index);
					}
					else
					{
						PortalSurfaceData->RemovePortal(Index);
						UE_LOG(LogTemp, Warning, TEXT("Portal cannot be placed at this location."));
					}
				}
			}
		}
		Destroy();
	}
}

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
void APortalBullet::OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	UE_LOG(LogTemp, Warning, TEXT("PORTAL HIT"));
}

/**
 * Finds the minimum and maximum vectors among the given points
 *
 * @param BR_Point Bottom-right point
 * @param BL_Point Bottom-left point
 * @param TR_Point Top-right point
 * @param TL_Point Top-left point
 * @return A pair containing the minimum and maximum vectors
 */
std::pair<FVector, FVector> APortalBullet::FindMinMax(const FVector& BR_Point, const FVector& BL_Point, const FVector& TR_Point, const FVector& TL_Point)
{
	float MinX = BR_Point.X;
	float MaxX = BR_Point.X;
	float MinY = BR_Point.Y;
	float MaxY = BR_Point.Y;

	FVector Points[] = { BL_Point, TR_Point, TL_Point };

	for (const FVector& Point : Points)
	{
		if (Point.X < MinX)
		{
			MinX = Point.X;
		}
		if (Point.X > MaxX)
		{
			MaxX = Point.X;
		}
		if (Point.Y < MinY)
		{
			MinY = Point.Y;
		}
		if (Point.Y > MaxY)
		{
			MaxY = Point.Y;
		}
	}

	FVector MinVector(MinX, MinY, 0.0f);
	FVector MaxVector(MaxX, MaxY, 0.0f);

	return std::make_pair(MinVector, MaxVector);
}

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
bool APortalBullet::AdjustPortalPosition(UPortalSurface* PortalSurfaceData, FVector& LocalPortalMin, FVector& LocalPortalMax, FVector& PortalCenter, const FVector& SurfaceOrigin, const FVector& SurfaceExtend, int32& Index, FVector& Displacement, FRotator& Rotation)
{
	bool bCanPortalBePlaced = true;

	FVector BoxMin = SurfaceOrigin - SurfaceExtend;
	FVector BoxMax = SurfaceOrigin + SurfaceExtend;

	Index = PortalSurfaceData->AddPortal(LocalPortalMin, LocalPortalMax, PortalCenter, Rotation);

	PortalSurfaceData->FitPortalToSurface(Index, BoxMin, BoxMax);

	if (!PortalSurfaceData->MovePortalOnOverlap(Index))
	{
		UE_LOG(LogTemp, Warning, TEXT("Moved Portal"))
		bCanPortalBePlaced = PortalSurfaceData->MovePortalOnOverlap(Index);
		if (!bCanPortalBePlaced)
		{
			UE_LOG(LogTemp, Warning, TEXT("Cancel Placement"))
			return bCanPortalBePlaced;
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Not Moved"))
	}

	FVector PortalCenterOld = PortalCenter;
	PortalSurfaceData->GetPortal(Index, LocalPortalMin, LocalPortalMax, PortalCenter);
	Displacement = PortalCenter - PortalCenterOld;
	return bCanPortalBePlaced;
}

/**
 * Spawns the portal on the surface
 *
 * @param PortalCenter The center point of the portal
 * @param PortalRotation The rotation of the portal
 * @param SurfaceForwardVector The forward vector of the surface
 * @param PortalSurfaceData The portal surface data component
 * @param Index The index of the portal
 */
void APortalBullet::SpawnPortalOnSurface(FVector PortalCenter, FQuat PortalRotation, FVector SurfaceForwardVector, UPortalSurface* PortalSurfaceData, int32 Index)
{
	SurfaceForwardVector *= 0.1; // offset from the surface, from testing, it needs to be 1.2 units apart or it might cause incorrect collision bugs. 
	TArray<AActor*> ManagerArray;
	if (BP_Portal3Manager != nullptr)
	{
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), BP_Portal3Manager, ManagerArray);
		APortal3Manager* PortalManager = Cast<APortal3Manager>(ManagerArray[0]);
		PortalManager->CreateNewPortal(PortalCenter + SurfaceForwardVector, PortalRotation, bIsOrangePortal, PortalSurfaceData, Index);
	}
}




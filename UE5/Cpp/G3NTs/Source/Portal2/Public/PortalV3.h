// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "Components/SceneCaptureComponent2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Components/BoxComponent.h"

#include "PortalV3.generated.h"

class USceneCaptureComponent2D;
class UTextureRenderTarget2D;
class UMaterialInstanceDynamic;
class UStaticMeshComponent;
class UPortalSurface;

UCLASS()
class PORTAL2_API APortalV3 : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	APortalV3();

public: 
	UPROPERTY(EditAnywhere, Category = "G3NTs|Portal")
	APortalV3* LinkedPortal;

	UPortalSurface* PortalSurface;
	bool bIsOrangePortal;

private:
	UPROPERTY(VisibleAnywhere, Category = "G3NTs|Portal")
	USceneCaptureComponent2D* SceneCapture; // essential

	UPROPERTY(VisibleAnywhere, Transient, Category = "G3NTs|Portal")
	UTextureRenderTarget2D* PortalTexture; // essential

	UPROPERTY(VisibleAnywhere, Transient, Category = "G3NTs|Portal")
	UTextureRenderTarget2D* PortalTexture2; // essential

	UPROPERTY(VisibleAnywhere, Category = "G3NTs|Portal")
	UStaticMeshComponent* PortalMesh; // essential

	UPROPERTY(VisibleAnywhere, Transient, Category = "G3NTs|Portal")
	UMaterialInstanceDynamic* DynamicMaterialInstance; // essential

	UPROPERTY(EditAnywhere, Category = "G3NTs|Portal")
	UMaterialInstance* Material; // essential

	UPROPERTY(EditAnywhere, Category = "G3NTs|Portal")
	FVector PortalEdgeColor; // essential

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Collision", meta = (AllowPrivateAccess = "true"))
	UBoxComponent* BoxCheck; // essential

	FVector PortalScale = FVector(2.4, 1.2, 1.2); // essential
	FRotator PortalRotation = FRotator(-90.f, 0.f, 0.f); // essential
	FVector2D OldSize; // essential
	bool bUsingPrimaryTextureTarget = true; // essential
	int32 SurfaceId; // essential

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

private:	
	/**
	 * Deprecated! Debugs the collider used for teleportation checks by drawing a debug box in the world.
	 */
	void DebugCollider();

	/**
	 * Resizes the static mesh to the defined portal scale.
	 */
	void ResizeStaticMesh(); /// worries me cause could have had a input param

	/**
	 * Draws a box in the world, ONLY when using the EDITOR. Used in combination with DebugCollider.
	 *
	 * @param World The world context.
	 * @param WorldOffset The offset of the box in the world.
	 * @param Color The color of the box.
	 * @param Duration The duration for which the box should be drawn.
	 */
	void DrawBox(UWorld* World, const FVector& WorldOffset, const FColor& Color, float Duration);

	/**
	 * Breaks a view projection matrix into its component vectors. 
	 * Which is then used in the portal material instance to correctly calculate the screen space coordinates.
	 *
	 * @param InMatrix The input matrix to break.
	 * @param OutX The output X vector.
	 * @param OutY The output Y vector.
	 * @param OutZ The output Z vector.
	 * @param OutW The output W vector.
	 */
	bool BreakMatrix(const FMatrix& InMatrix, FVector4& OutX, FVector4& OutY, FVector4& OutZ, FVector4& OutW);

public:
	/**
	 * Updates the screen capture resources.
	 */
	void NullScreenCapture();

	/**
	 * Destroys the portal and removes its link to the linked portal.
	 */
	void PortalDestroySelf();

	/**
	 * Updates the screen capture with new parameters. 
	 *
	 * @param Position The new position for the screen capture.
	 * @param Rotation The new rotation for the screen capture.
	 * @param ViewProjectionMatrix The new view projection matrix.
	 * @param Target The target portal transform.
	 * @param ProjectionMatrix The projection matrix.
	 */
	void UpdateScreenCapture(FVector Position, FQuat Rotation, FMatrix ViewProjectionMatrix, FTransform Target, FMatrix ProjectionMatrix);
	
	/**
	 * Updates the 2 texture targets. If the texture target objects do not yet exist, 
	 * it creates them and sets their default values. If the texture target objects do exist, 
	 * checks if the screen size changed, and if so updates the texture size.
	 *
	 * @param Size The new size for the texture target.
	 */
	void UpdateTextureTarget(FVector2D Size);
	
	/**
	 * Sets the surface data for the portal. The surface data, is a reference to the surface static mesh, 
	 * which this portal is placed on.
	 *
	 * @param Index The index of the surface.
	 * @param PortalSurfaceData The surface data pointer.
	 */
	void SetSurfaceData(int32 Index, UPortalSurface* PortalSurfaceData);

	/**
	 * Sets the portal color edge color.
	 *
	 * @param ColorIn The new color for the portal edge.
	 */
	void SetPortalColor(FVector ColorIn);

	/**
	 * Gets the coordinates of the bounds of the portal. 
	 * However, it discards one dimension as the portal plane is a "plane" not a box.
	 *
	 * @return An array of vectors representing the bounds of the portal.
	 */
	TArray<FVector> GetPortalBounds();

	/**
	 * Checks if a point is inside the portal collider box.
	 * In order to work with the possible rotations, normalizes the rotation and location to absolute local coordinates.
	 *
	 * @param Point The point to check.
	 * @return True if the point is inside the portal, false otherwise.
	 */
	bool IsInside(FVector Point);

};

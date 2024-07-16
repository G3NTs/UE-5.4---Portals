// Fill out your copyright notice in the Description page of Project Settings.


#include "PortalV3.h"
#include "PortalSurface.h"
#include "Math/UnrealMathUtility.h"

// Sets default values
APortalV3::APortalV3()
{
    PrimaryActorTick.bCanEverTick = false;

    /**
     * Initializing all the components connected to the portal actor 
     */
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
	RootComponent->Mobility = EComponentMobility::Movable;

	PortalMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Portal"));
	PortalMesh->SetupAttachment(RootComponent);

    /**
     * We use a construction helper to find the portal plane asset.
     * This might not be the best way going around. A better way would have been to have 
     * a staticmesh component and select the reference shape it in the editor. 
     * This possible prevents overwriting the original plane asset.
     */
	static ConstructorHelpers::FObjectFinder<UStaticMesh> PlaneMeshAsset(TEXT("StaticMesh'/Game/StarterContent/Shapes/Shape_Plane.Shape_Plane'"));
	if (PlaneMeshAsset.Succeeded())
	{
		PortalMesh->SetStaticMesh(PlaneMeshAsset.Object);
		PortalMesh->SetRelativeLocation(FVector(0, 0, 0));
		PortalMesh->SetRelativeRotation(PortalRotation); // y,z,x
		PortalMesh->SetRelativeScale3D(PortalScale);
        PortalMesh->SetCollisionProfileName(TEXT("NoCollision"));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("PortalMesh is a nullptr"));
	}

    /**
     * We also generate a box component around the portal plane. 
     * this is used to check for player teleportation. The collision response is set to overlap as 
     * no hard solid collisions are required. 
     */
    BoxCheck = CreateDefaultSubobject<UBoxComponent>(TEXT("BoxCollider"));
    BoxCheck->SetRelativeScale3D(PortalRotation.RotateVector(PortalScale));
    BoxCheck->SetBoxExtent(FVector(100,50,50));

    BoxCheck->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    BoxCheck->SetCollisionResponseToAllChannels(ECR_Ignore);
    BoxCheck->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Overlap);

    /**
     * Creating a USceneCaptureComponent which is basically a virtual camera,
     * used for capturing what the player sees through the portal.
     */
    SceneCapture = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("PortalSceneCapture"));
    SceneCapture->SetupAttachment(RootComponent);

    SceneCapture->bCaptureEveryFrame = false;
    SceneCapture->bCaptureOnMovement = false;
    SceneCapture->LODDistanceFactor = 3;
    SceneCapture->TextureTarget = nullptr;
    SceneCapture->bEnableClipPlane = true;
    SceneCapture->bUseCustomProjectionMatrix = true;
    SceneCapture->CaptureSource = ESceneCaptureSource::SCS_SceneColorHDRNoAlpha;
    SceneCapture->bAlwaysPersistRenderingState = true;

    SceneCapture->PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_RenderScenePrimitives;

    //Setup Post-Process of SceneCapture (optimization : disable Motion Blur, etc)
    FPostProcessSettings CaptureSettings;

    CaptureSettings.bOverride_AmbientOcclusionQuality = true;
    CaptureSettings.bOverride_MotionBlurAmount = true;
    CaptureSettings.bOverride_SceneFringeIntensity = true;
    CaptureSettings.bOverride_ScreenSpaceReflectionQuality = true;

    CaptureSettings.AmbientOcclusionQuality = 0.0f;
    CaptureSettings.MotionBlurAmount = 0.0f;
    CaptureSettings.SceneFringeIntensity = 0.0f;
    CaptureSettings.ScreenSpaceReflectionQuality = 0.0f;

    // Additional overrides
    CaptureSettings.bOverride_AutoExposureMinBrightness = true;
    CaptureSettings.bOverride_AutoExposureMaxBrightness = true;
    CaptureSettings.AutoExposureMinBrightness = 0.03f;
    CaptureSettings.AutoExposureMaxBrightness = 2.0f;
    CaptureSettings.bOverride_AutoExposureBias = true;
    CaptureSettings.AutoExposureBias = 0.0f;
    CaptureSettings.bOverride_AutoExposureLowPercent = true;
    CaptureSettings.AutoExposureLowPercent = 10.0f;
    CaptureSettings.bOverride_AutoExposureHighPercent = true;
    CaptureSettings.AutoExposureHighPercent = 90.0f;

    SceneCapture->PostProcessSettings = CaptureSettings;
}

void APortalV3::BeginPlay()
{
	Super::BeginPlay();

    /**
     * During Beginplay, the material instances are created and set. 
     * So that each portal has a unique material reference. 
     */

    DynamicMaterialInstance = UMaterialInstanceDynamic::Create(Material, this);
    DynamicMaterialInstance->SetFlags(RF_Transient);

	PortalMesh->SetMaterial(0, DynamicMaterialInstance);

    UpdateTextureTarget(FVector2D(512, 512));

    SceneCapture->TextureTarget = PortalTexture;

    DynamicMaterialInstance->SetTextureParameterValue(TEXT("Texture"), PortalTexture);
    DynamicMaterialInstance->SetVectorParameterValue(TEXT("PortalEdge"), PortalEdgeColor);
}

/**
 * Updates the screen capture with new parameters.
 *
 * @param Position The new position for the screen capture.
 * @param Rotation The new rotation for the screen capture.
 * @param ViewProjectionMatrix The new view projection matrix.
 * @param Target The target portal transform.
 * @param ProjectionMatrix The projection matrix.
 */
void APortalV3::UpdateScreenCapture(FVector NewLocation, FQuat NewRotation, FMatrix ViewProjectionMatrix, FTransform Target, FMatrix ProjectionMatrix)
{
    ViewProjectionMatrix = ViewProjectionMatrix.GetTransposed();

    FVector4 VecX, VecY, VecZ, VecW;
    BreakMatrix(ViewProjectionMatrix, VecX, VecY, VecZ, VecW);

    DynamicMaterialInstance->SetVectorParameterValue(TEXT("VPX"), FVector4(VecX.X, VecX.Y, VecX.Z, VecX.W));
    DynamicMaterialInstance->SetVectorParameterValue(TEXT("VPY"), FVector4(VecY.X, VecY.Y, VecY.Z, VecY.W));
    DynamicMaterialInstance->SetVectorParameterValue(TEXT("VPW"), FVector4(VecW.X, VecW.Y, VecW.Z, VecW.W));

    SceneCapture->SetWorldLocation(NewLocation);
    SceneCapture->SetWorldRotation(NewRotation);

    SceneCapture->ClipPlaneNormal = Target.GetRotation().GetForwardVector();
    SceneCapture->ClipPlaneBase = Target.GetLocation() + (SceneCapture->ClipPlaneNormal * -1.5f);

    SceneCapture->CustomProjectionMatrix = ProjectionMatrix;

    SceneCapture->CaptureScene();

    /**
     * Two Textures are used to alternate. This help creating semi-recursion for the portals. 
     * As one Texture can not display itself via a scenecapture component.
     */
	if (bUsingPrimaryTextureTarget)
	{
		SceneCapture->TextureTarget = PortalTexture;
		bUsingPrimaryTextureTarget = false;
		DynamicMaterialInstance->SetTextureParameterValue(TEXT("Texture"), PortalTexture2);
	}
	else
	{
		SceneCapture->TextureTarget = PortalTexture2;
		bUsingPrimaryTextureTarget = true;
		DynamicMaterialInstance->SetTextureParameterValue(TEXT("Texture"), PortalTexture);
	}
}

/**
 * Updates the screen capture resources.
 */
void APortalV3::NullScreenCapture()
{
    PortalTexture->UpdateResource();
    PortalTexture2->UpdateResource();
}
/**
 * Updates the 2 texture targets. If the texture target objects do not yet exist,
 * it creates them and sets their default values. If the texture target objects do exist,
 * checks if the screen size changed, and if so updates the texture size.
 *
 * @param Size The new size for the texture target.
 */
void APortalV3::UpdateTextureTarget(FVector2D Size)
{
	if (PortalTexture == nullptr && PortalTexture2 == nullptr)
	{
		PortalTexture = NewObject<UTextureRenderTarget2D>(this, UTextureRenderTarget2D::StaticClass(), *FString("PortalRenderTarget"));
		PortalTexture2 = NewObject<UTextureRenderTarget2D>(this, UTextureRenderTarget2D::StaticClass(), *FString("PortalRenderTarget2"));

		PortalTexture->RenderTargetFormat = ETextureRenderTargetFormat::RTF_RGBA8_SRGB;
		PortalTexture->Filter = TextureFilter::TF_Default;
		PortalTexture->ClearColor = FColor::Black;
		PortalTexture->bNeedsTwoCopies = false;
		PortalTexture->AddressX = TextureAddress::TA_Clamp;
		PortalTexture->AddressY = TextureAddress::TA_Clamp;

		PortalTexture2->RenderTargetFormat = ETextureRenderTargetFormat::RTF_RGBA8_SRGB;
		PortalTexture2->Filter = TextureFilter::TF_Default;
		PortalTexture2->ClearColor = FColor::Black;
		PortalTexture2->bNeedsTwoCopies = false;
		PortalTexture2->AddressX = TextureAddress::TA_Clamp;
		PortalTexture2->AddressY = TextureAddress::TA_Clamp;

		PortalTexture->SizeX = 1524;
		PortalTexture->SizeY = FMath::RoundToInt(1524 * Size.Y / Size.X);

		PortalTexture2->SizeX = 1524;
		PortalTexture2->SizeY = FMath::RoundToInt(1524 * Size.Y / Size.X);

		OldSize = Size;

		PortalTexture->UpdateResource();
		PortalTexture2->UpdateResource();
	}
    else if (Size != OldSize && PortalTexture != nullptr && PortalTexture2 != nullptr)
    {
        OldSize = Size;

        PortalTexture->SizeX = 1524;
        PortalTexture->SizeY = FMath::RoundToInt(1524 * Size.Y / Size.X);
        PortalTexture2->SizeX = 1524;
        PortalTexture2->SizeY = FMath::RoundToInt(1524 * Size.Y / Size.X);

        PortalTexture->UpdateResource();
        PortalTexture2->UpdateResource();
    }
}

/**
 * Deprecated! Debugs the collider used for teleportation checks by drawing a debug box in the world.
 */
void APortalV3::DebugCollider()
{
    DrawBox(GetWorld(), GetActorLocation(), FColor::Purple, -1);
}

/**
 * Gets the coordinates of the bounds of the portal.
 * However, it discards one dimension as the portal plane is a "plane" not a box.
 *
 * @return An array of vectors representing the bounds of the portal.
 */
TArray<FVector> APortalV3::GetPortalBounds()
{
    TArray<FVector> PortalBounds;

    FBox MeshBox = PortalMesh->GetStaticMesh()->GetBounds().GetBox();
    FTransform Transform(PortalRotation + FRotator(0.f, 90.f, 0.f), GetActorLocation(), PortalScale);

    /**
     * iterating through all corners of a standard bounds box.
     */
    for (int32 i = 0; i < 8; i++)
    {
        FVector LocalCorner(
            (i & 1) ? MeshBox.Max.X : MeshBox.Min.X,
            (i & 2) ? MeshBox.Max.Y : MeshBox.Min.Y,
            MeshBox.Min.Z
        );
        PortalBounds.Add(Transform.TransformPosition(LocalCorner));
    }
    return PortalBounds;
}

/**
 * Resizes the static mesh to the defined portal scale.
 */
void APortalV3::ResizeStaticMesh()
{
    PortalMesh->SetRelativeScale3D(PortalScale);
}

/**
 * Draws a box in the world, ONLY when using the EDITOR. Used in combination with DebugCollider.
 *
 * @param World The world context.
 * @param WorldOffset The offset of the box in the world.
 * @param Color The color of the box.
 * @param Duration The duration for which the box should be drawn.
 */
void APortalV3::DrawBox(UWorld* World, const FVector& WorldOffset, const FColor& Color, float Duration)
{
    if (GEngine)
    {
        FVector BoxCenter = BoxCheck->GetComponentLocation();
        FVector BoxExtent = BoxCheck->GetScaledBoxExtent();

        DrawDebugBox(GetWorld(), BoxCenter + GetActorLocation(), BoxExtent, GetActorQuat(), Color, false, Duration, 0, 1);
    }
}

/**
 * Checks if a point is inside the portal collider box. 
 * In order to work with the possible rotations, normalizes the rotation and location to absolute local coordinates.
 *
 * @param Point The point to check.
 * @return True if the point is inside the portal, false otherwise.
 */
bool APortalV3::IsInside(FVector Point)
{
    FVector Offset = GetActorLocation();
    FVector BoxCenter = BoxCheck->GetComponentLocation();
    FVector BoxExtent = BoxCheck->GetScaledBoxExtent();

    FVector DirectionX = BoxCheck->GetForwardVector();
    FVector DirectionY = BoxCheck->GetRightVector();
    FVector DirectionZ = BoxCheck->GetUpVector();

    FVector Direction = Point - Offset;

    Direction = GetActorRotation().GetInverse().RotateVector(Direction);

    bool IsInside = FMath::Abs(Direction.X) <= BoxExtent.X && FMath::Abs(Direction.Y) <= BoxExtent.Y && FMath::Abs(Direction.Z) <= -BoxExtent.Z;

    return IsInside;
}

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
bool APortalV3::BreakMatrix(const FMatrix& InMatrix, FVector4& OutX, FVector4& OutY, FVector4& OutZ, FVector4& OutW)
{
    if (sizeof(InMatrix.M) / sizeof(InMatrix.M[0]) != 4 || sizeof(InMatrix.M[0]) / sizeof(int) != 8)
    {
        return false;
    }

    OutX = FVector4(InMatrix.M[0][0], InMatrix.M[0][1], InMatrix.M[0][2], InMatrix.M[0][3]);
    OutY = FVector4(InMatrix.M[1][0], InMatrix.M[1][1], InMatrix.M[1][2], InMatrix.M[1][3]);
    OutZ = FVector4(InMatrix.M[2][0], InMatrix.M[2][1], InMatrix.M[2][2], InMatrix.M[2][3]);
    OutW = FVector4(InMatrix.M[3][0], InMatrix.M[3][1], InMatrix.M[3][2], InMatrix.M[3][3]);

    return true;
}

/**
 * Destroys the portal and removes its link to the linked portal.
 */
void APortalV3::PortalDestroySelf()
{
    if (LinkedPortal != nullptr)
    {
        LinkedPortal->LinkedPortal = nullptr;
        UE_LOG(LogTemp, Warning, TEXT("Destroying the link"))
    }
    PortalSurface->RemovePortal(SurfaceId);
    Destroy();
}

/**
 * Sets the surface data for the portal. The surface data, is a reference to the surface static mesh,
 * which this portal is placed on.
 *
 * @param Index The index of the surface.
 * @param PortalSurfaceData The surface data pointer.
 */
void APortalV3::SetSurfaceData(int32 Index, UPortalSurface* PortalSurfaceData)
{
    PortalSurface = PortalSurfaceData;
    SurfaceId = Index;
}

/**
 * Sets the portal color edge color.
 *
 * @param ColorIn The new color for the portal edge.
 */
void APortalV3::SetPortalColor(FVector ColorIn)
{
    PortalEdgeColor = ColorIn;
    DynamicMaterialInstance->SetVectorParameterValue(TEXT("PortalEdge"), PortalEdgeColor);
}


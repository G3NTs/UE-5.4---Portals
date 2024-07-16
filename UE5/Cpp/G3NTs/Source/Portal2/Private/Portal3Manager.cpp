// Fill out your copyright notice in the Description page of Project Settings.


#include "Portal3Manager.h"
#include "C:/Users/G3NTs/Documents/Unreal Projects/Portal2/Source/Portal2/Portal2Projectile.h"
#include "C:/Users/G3NTs/Documents/Unreal Projects/Portal2/Source/Portal2/Portal2Character.h"
#include "C:/Users/G3NTs/Documents/Unreal Projects/Portal2/Source/Portal2/TP_WeaponComponent.h"
#include "Camera/CameraComponent.h"
#include "EnhancedInputComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputSubsystems.h"
#include "MyAnimInstance.h"
#include "Animation/AnimInstance.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "PortalSurface.h"
#include "SceneView.h"

APortal3Manager::APortal3Manager()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PostUpdateWork;

	SecondaryActorTick.bCanEverTick = true;
	SecondaryActorTick.TickGroup = TG_PostPhysics;

	bCloneState = false;
}

void APortal3Manager::BeginPlay()
{
	Super::BeginPlay();

	PlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);

	// problem for shipping build: viewport size is zero for first view frames
	UpdateViewportSize();
}

void APortal3Manager::RegisterActorTickFunctions(bool bRegister)
{
	Super::RegisterActorTickFunctions(bRegister);

	/*
	 * Two Tick functions are registered, we run different functions at different steps between frames. 
	 * This fixes several issues related to update delays with the portal scene capture components
	 */
	if (bRegister)
	{
		if (SecondaryActorTick.bCanEverTick)
		{
			SecondaryActorTick.Target = this;
			SecondaryActorTick.RegisterTickFunction(GetLevel());
		}
	}
	else
	{
		if (SecondaryActorTick.IsTickFunctionRegistered())
		{
			SecondaryActorTick.UnRegisterTickFunction();
		}
	}
}

void APortal3Manager::TickActor(float DeltaSeconds, ELevelTick TickType, FActorTickFunction& ThisTickFunction)
{
	Super::TickActor(DeltaSeconds, TickType, ThisTickFunction);

	/*
	 * Teleportation checks are done before any updates of the Scenecapture components.
	 */
	if (ThisTickFunction.TickGroup == TG_PostPhysics)
	{
		TeleportActorsCheck();
	}
	else if (ThisTickFunction.TickGroup == TG_PostUpdateWork)
	{
		CloneOrUpdateAllActors();
		ResetRotationControllerSlerp(DeltaSeconds);
		UpdatePortals();
	}
}

/**
 * Finds all actors of a specified subclass in the world that inherit from a given base class.
 * This function uses UGameplayStatics::GetAllActorsOfClass to gather all actors of the specified BaseClass,
 * then attempts to cast each found actor to SubClass. If successful, it adds the casted actor pointer to
 * an array which is returned at the end.
 *
 * @tparam SubClass The subclass of AActor to search for.
 * @param World The world context to search for actors.
 * @param BaseClass The base class from which SubClass inherits.
 * @return An array of pointers to actors of SubClass found in the world. May be empty if none are found.
 */
template<typename SubClass>
TArray<SubClass*> APortal3Manager::FindAllActorsInWorld(UWorld* World, TSubclassOf<AActor> BaseClass)
{
	TArray<SubClass*> ActorList;
	TArray<AActor*> Actors;
	UGameplayStatics::GetAllActorsOfClass(World, BaseClass, Actors);

	for (AActor* Actor : Actors)
	{
		if (SubClass* ActorSubclass = Cast<SubClass>(Actor))
		{
			ActorList.Add(ActorSubclass);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("Casting BaseClass Actor to SubClass Failed! - Make sure to set the correct BaseClass"));
		}
	}

	return ActorList;
}

/**
 * Updates the capture state of all portals in the PortalList.
 *
 * This function iterates through each portal in the PortalList and checks if the portal has a linked portal.
 * If a linked portal is found, it calculates the necessary transforms and updates the portal's screen capture
 * to reflect the current state of the camera relative to the portal and its linked portal.
 */
void APortal3Manager::UpdatePortals()
{
	for (APortalV3* Portal : PortalList)
	{
		if (Portal->LinkedPortal == nullptr)
		{
			Portal->NullScreenCapture();
			continue;
		}

		FTransform PortalTransform = Portal->GetActorTransform();
		FTransform CameraTransform = PlayerController->PlayerCameraManager->GetTransform();

		if (CheckPortalNeedsUpdate(Portal, PortalTransform, CameraTransform))
		{
			FTransform TargetTransform = Portal->LinkedPortal->GetActorTransform();

			UpdatePortalCapture(Portal, PortalTransform, TargetTransform, CameraTransform);
		}
	}
}

/**
 * Updates the screen capture for the specified portal.
 *
 * This function calculates the new capture location and rotation for the portal based on the provided
 * reference, target, and camera transforms. It then retrieves the camera's view projection and projection
 * matrices, and uses these values to update the portal's screen capture.
 *
 * @param Portal The portal to update.
 * @param Reference The reference transform used for coordinate conversion, the main portal.
 * @param Target The transform of the linked portal.
 * @param Camera The current transform of the camera.
 */
void APortal3Manager::UpdatePortalCapture(APortalV3* Portal, FTransform Reference, FTransform Target, FTransform Camera)
{
	APlayerCameraManager* CameraManager = PlayerController->PlayerCameraManager;

	FVector CaptureLocation = ConvertLocationToActorSpace(Camera, Reference, Target);
	FQuat CaptureRotation = ConvertRotationToActorSpace(Camera, Reference, Target);
	FMatrix ViewProjectionMatrix = GetCameraProjectionMatrix(CameraManager, true);
	FMatrix ProjectionMatrix = GetCameraProjectionMatrix(CameraManager, false);

	Portal->UpdateScreenCapture(CaptureLocation, CaptureRotation , ViewProjectionMatrix, Target, ProjectionMatrix);
}

/**
 * Checks and manages teleportation for each agent in the TeleportAgents map.
 * Iterates through each agent and evaluates its position relative to portals in the PortalList.
 * Manages teleportation status, collision settings, and clip plane for each teleport agent.
 */
void APortal3Manager::TeleportActorsCheck()
{
	for (TPair<AActor*, AActor*>& Pair : TeleportAgents)
	{
		AActor* Agent = Pair.Value; // sometimes creates a nullptr error, no clue why. temporary fix: skip it...
		if (Agent == nullptr)
		{
			UE_LOG(LogTemp, Warning, TEXT("TeleportAgents Length: %d"), TeleportAgents.Num())
			continue;
		}
		UTeleportAgent* TeleportAgent = Agent->FindComponentByClass<UTeleportAgent>();
		bool bIsInsideAny = false; // a bool used to keep track wether a agent is not in any of the portal box colliders, prevents overwriting variables for multiple portals

		for (APortalV3* Portal: PortalList)
		{
			if (Portal->LinkedPortal == nullptr)
			{
				// if the portal has no linked portal skip to the next portal
				continue;
			}

			// series of checks to check the state of the teleport agent, for more info, check the function descriptions by hovering, or by peek definition.
			if (Portal->IsInside(Agent->GetActorLocation()))
			{
				if (!TeleportAgent->bDoNotTeleport)
				{
					TeleportAgent->ChangeAgentCollision(false);
				}
				
				// Check if the actor stays in front of the portal when inside the portal collider. if a change is detected, teleport.
				if (CheckActorInFront(Portal->GetActorTransform(), Agent->GetActorTransform()))
				{
					TeleportAgent->SetTeleportStatus(Portal, true);
					TeleportAgent->SetClipPlane(Portal->GetActorLocation(), Portal->GetActorTransform().GetRotation().GetForwardVector());
					bIsInsideAny = true;
				}
				else if (TeleportAgent->GetTeleportStatus(Portal))
				{
					TeleportAgent->SetTeleportStatus(Portal, false);
					TeleportActor(Agent, Portal);
					TeleportAgent->SetTeleportStatus(Portal->LinkedPortal, true);
					TeleportAgent->SetClipPlane(Portal->LinkedPortal->GetActorLocation(), Portal->LinkedPortal->GetActorTransform().GetRotation().GetForwardVector());
					bIsInsideAny = true;
				}
			}
			else
			{
				TeleportAgent->SetTeleportStatus(Portal, false);
			}
			if (!bIsInsideAny)
			{
				TeleportAgent->DisableClipPlane();
				TeleportAgent->ResetAgentCollision();
			}
		}
	}
}

/** 
 * Deprecated! No longer used in the final version of the code 
 */
void APortal3Manager::UpdateDebugDisplay(ADebugDisplay* DebugDisplayActorIn)
{
	DebugDisplayActorIn->bTick = true;
}

/**
 * Resets the control and actor rotations of the player character smoothly using spherical linear interpolation (SLERP).
 * This function corrects the player's orientation after passing through portals that may have altered their control rotation.
 * The function uses Quaternions, as they are less prone to interlocking rotations and such.
 * 
 * @param DeltaTime The time elapsed since the last frame.
 */
void APortal3Manager::ResetRotationControllerSlerp(float DeltaTime)
{
	FQuat PlayerControlQuat = PlayerController->GetControlRotation().Quaternion();
	FQuat PlayerActorQuat = PlayerController->GetCharacter()->GetActorRotation().Quaternion();

	FRotator PlayerControlRot = PlayerController->GetControlRotation();
	FRotator PlayerActorRot = PlayerController->GetCharacter()->GetActorRotation();

	float AdjustmentSpeed1, AdjustmentSpeed2;

	AdjustmentSpeed1 = 4.0f;
	AdjustmentSpeed2 = 2.0f;

	FRotator TargetControlRot = PlayerControlRot;
	FRotator TargetActorRot = PlayerActorRot;

	TargetControlRot.Roll = 0.0f;

	TargetActorRot.Roll = 0.0f;
	TargetActorRot.Pitch = 0.0f;

	FQuat TargetControlQuat = FQuat(TargetControlRot);
	FQuat TargetActorQuat = FQuat(TargetActorRot);

	FQuat NewControlQuat = FQuat::Slerp(PlayerControlQuat, TargetControlQuat, AdjustmentSpeed1 * DeltaTime);
	FQuat NewActorQuat = FQuat::Slerp(PlayerActorQuat, TargetActorQuat, AdjustmentSpeed2 * DeltaTime);

	PlayerController->SetControlRotation(NewControlQuat.Rotator());
	PlayerController->GetCharacter()->SetActorRotation(NewActorQuat.Rotator());

	const float Tolerance = 0.1; // or a small threshold value
	if (NewControlQuat.Equals(TargetControlQuat, Tolerance) && NewActorQuat.Equals(TargetActorQuat, Tolerance))
	{
		PlayerController->GetCharacter()->bUseControllerRotationYaw = true;
		PlayerController->GetCharacter()->bUseControllerRotationRoll = true;
	}
}

/**
 * Iterates through all teleport agents and checks their teleport status for each portal.
 * Clones or updates actors based on teleport status. Removes cloned actors if teleport
 * status is false for a portal.
 * Could possibly have been combined with Teleport Actors Check function!
 */
void APortal3Manager::CloneOrUpdateAllActors()
{
	bCloneState = true;
	for (TPair<AActor*, AActor*>& Pair : TeleportAgents)
	{
		AActor* Agent = Pair.Value; // sometimes creates a nullptr error
		if (Agent == nullptr)
		{
			UE_LOG(LogTemp, Warning, TEXT("TeleportAgents Length: %d"), TeleportAgents.Num())
				continue;
		}
		UTeleportAgent* TeleportAgent = Agent->FindComponentByClass<UTeleportAgent>(); 

		for (APortalV3* Portal : PortalList)
		{
			if (Portal->LinkedPortal == nullptr)
			{
				continue;
			}
			if (TeleportAgent->GetTeleportStatus(Portal))
			{
				UE_LOG(LogTemp, Warning, TEXT("01 Clone or Update Actor"));
				CloneOrUpdateActor(Agent, Portal);
			}
			else
			{
				RemoveClonedActor(Agent, Portal);
			}
		}
	}
	bCloneState = false;
}

/**
 * Updates the viewport size and applies it to all portals or a single one.
 *
 * @param APortalV3 Reference, if provided, the function will only update the specific portal reference.
 * @return True if the viewport size was successfully retrieved, false if default size was used.
 */
bool APortal3Manager::UpdateViewportSize(APortalV3* Portal)
{
	bool bIsViewportSucces = true;
	FVector2D ViewportSize;
	UGameViewportClient* ViewportClient = GetWorld()->GetGameViewport();
	ViewportClient->GetViewportSize(ViewportSize);

	MapAllActorsWithComponent(GetWorld(), TeleportAgents);
	PortalList = FindAllActorsInWorld<APortalV3>(GetWorld(), ABP_PortalV2);

	if (ViewportSize.X == 0 || ViewportSize.Y == 0)
	{
		ViewportSize = FVector2D(256, 256); 
		bIsViewportSucces = false;
	}

	if (Portal == nullptr)
	{
		for (APortalV3* Portal_i : PortalList)
		{
			Portal_i->UpdateTextureTarget(ViewportSize);
		}
	}
	else
	{
		Portal->UpdateTextureTarget(ViewportSize);
	}
	return bIsViewportSucces;
}

/**
 * Clones or updates the specified agent in the context of the given portal.
 *
 * This function checks if the agent already has a cloned counterpart associated with the portal.
 * If no cloned actor is found, it creates a new clone of the agent.
 * If a cloned actor exists, it updates the cloned actor's state to match the agent.
 * Finally, it sets the clip plane on the cloned actor's teleport agent component based on the linked portal's location and forward vector.
 *
 * @param Agent The actor to be cloned or updated.
 * @param Portal The portal that influences the cloning or updating process.
 */
void APortal3Manager::CloneOrUpdateActor(AActor* Agent, APortalV3* Portal)
{
	if (!Agent || !Portal || !Portal->LinkedPortal)
	{
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("02 Find Actor pointer in ClonedActors Map"));
	
	AActor* ClonedActor = FindClonedActor(Agent, Portal);

	if (ClonedActor == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("03 Cloning Actor"));
		CloneActor(Agent, Portal);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("06 Updating Actor"));
		UpdateClonedActor(Agent, ClonedActor, Portal);
	}
	ClonedActor = FindClonedActor(Agent, Portal);
	UTeleportAgent* ClonedTeleportAgent = ClonedActor->FindComponentByClass<UTeleportAgent>();
	ClonedTeleportAgent->SetClipPlane(Portal->LinkedPortal->GetActorLocation(), Portal->LinkedPortal->GetActorTransform().GetRotation().GetForwardVector());
}

/**
 * Clones the specified actor through the given portal.
 * Depending on the type of actor (player character, projectile, or static mesh),
 * it handles cloning and transformation appropriately.
 * 
 * @param Agent The actor to be cloned.
 * @param Portal The portal through which the actor will be cloned.
 */
void APortal3Manager::CloneActor(AActor* Agent, APortalV3* Portal)
{
	FTransform TargetTransform = Portal->LinkedPortal->GetActorTransform();
	FTransform PortalTransform = Portal->GetActorTransform();
	FTransform ActorTransform = Agent->GetActorTransform();

	UTeleportAgent* TeleportAgent = Agent->FindComponentByClass<UTeleportAgent>();

	FVector NewLocation = ConvertLocationToActorSpace(ActorTransform, PortalTransform, TargetTransform);
	FQuat NewRotation = ConvertRotationToActorSpace(ActorTransform, PortalTransform, TargetTransform);
	FTransform NewTransform(NewRotation, NewLocation, ActorTransform.GetScale3D());

	TArray<AActor*> AttachedActors;
	Agent->GetAttachedActors(AttachedActors);

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	UE_LOG(LogTemp, Warning, TEXT("04 Cloning Actor"));

	if (TeleportAgent->bIsPlayerController)
	{
		FQuat NewRotationCam = ConvertRotationToActorSpace(PlayerController->PlayerCameraManager->GetTransform(), PortalTransform, TargetTransform);

		ACharacter* ClonedCharacter = GetWorld()->SpawnActor<ACharacter>(Agent->GetClass(), NewLocation, NewRotation.Rotator(), SpawnParams);
		APortal2Character* ClonedPortal2Character = Cast<APortal2Character>(ClonedCharacter);
		APortal2Character* AgentPortal2Character = Cast<APortal2Character>(Agent);

		if (!ClonedCharacter)
		{
			UE_LOG(LogTemp, Error, TEXT("Cloning Actor Failed!"));
		}

		UCameraComponent* CameraComponent = ClonedPortal2Character->GetFirstPersonCameraComponent();

		USkeletalMeshComponent* ArmsComponent = ClonedPortal2Character->GetMesh1P();
		USkeletalMeshComponent* AgentArmsComponent = AgentPortal2Character->GetMesh1P();

		if (CameraComponent && ArmsComponent)
		{
			UMyAnimInstance* AnimInstanceMain = Cast<UMyAnimInstance>(AgentArmsComponent->GetAnimInstance());
			UMyAnimInstance* AnimInstance = Cast<UMyAnimInstance>(ArmsComponent->GetAnimInstance());

			AnimInstance->bOverrideBools = true;
			AnimInstance->bUpdateBools = true;
			AnimInstance->bIsMoving = AnimInstanceMain->bIsMoving;
			AnimInstance->bIsInAir = AnimInstanceMain->bIsInAir;
			AnimInstance->bHasRifle = AnimInstanceMain->bHasRifle;
			AnimInstance->bTransitionDown = AnimInstanceMain->bTransitionDown;
			AnimInstance->bTransitionUp = AnimInstanceMain->bTransitionUp;
			AnimInstance->StartPosition = AnimInstanceMain->OutPosition;

			ClonedCharacter->GetCharacterMovement()->Velocity = ConvertVelocityToActorSpace(AgentPortal2Character->GetCharacterMovement()->Velocity, PortalTransform, TargetTransform);

			CameraComponent->SetWorldRotation(NewRotationCam);
		}

		StoreClonedActor(Agent, Portal, ClonedCharacter);

		for (AActor* AttachedActor : AttachedActors)
		{
			AActor* ClonedAttachedActor = GetWorld()->SpawnActor<AActor>(AttachedActor->GetClass(), NewLocation, NewRotation.Rotator(), SpawnParams);
			UTP_WeaponComponent* WeaponComponent = ClonedAttachedActor->FindComponentByClass<UTP_WeaponComponent>();
			if (WeaponComponent)
			{
				WeaponComponent->AttachWeapon(ClonedPortal2Character);
			}
		}
	}
	else if (Agent->IsA(APortal2Projectile::StaticClass()))
	{
		APortal2Projectile* ClonedProjectile = GetWorld()->SpawnActor<APortal2Projectile>(Agent->GetClass(), NewLocation, NewRotation.Rotator(), SpawnParams);

		ClonedProjectile->SetProjectileMovement(ConvertVelocityToActorSpace(Cast<APortal2Projectile>(Agent)->GetVelocity(), PortalTransform, TargetTransform));

		ClonedProjectile->SetActorEnableCollision(ECollisionEnabled::NoCollision);

		StoreClonedActor(Agent, Portal, ClonedProjectile);
	}
	else if (Agent->IsA(AActor::StaticClass()))
	{
		AActor* ClonedStaticMesh = GetWorld()->SpawnActor<AActor>(Agent->GetClass(), NewLocation, NewRotation.Rotator(), SpawnParams);
		ClonedStaticMesh->SetActorTransform(NewTransform);

		UPrimitiveComponent* PrimitiveComponent = Cast<UPrimitiveComponent>(ClonedStaticMesh->GetRootComponent());
		if (PrimitiveComponent)
		{
			PrimitiveComponent->SetPhysicsLinearVelocity(ConvertVelocityToActorSpace(Cast<AActor>(Agent)->GetVelocity(), PortalTransform, TargetTransform));
		}
		StoreClonedActor(Agent, Portal, ClonedStaticMesh);
	}
	UE_LOG(LogTemp, Warning, TEXT("05 Cloned Actor"));
}

/**
 * Updates the cloned actor's position, rotation, and other properties based on the agent's state
 * and the portal through which it was cloned. Depending on the type of actor (player character, projectile, or static mesh),
 * it handles cloning and transformation appropriately.
 * 
 * @param Agent The original actor whose clone is being updated.
 * @param ClonedActor The cloned actor that needs to be updated.
 * @param Portal The portal through which the actor was cloned.
 */
void APortal3Manager::UpdateClonedActor(AActor* Agent, AActor* ClonedActor, APortalV3* Portal)
{
	FTransform TargetTransform = Portal->LinkedPortal->GetActorTransform();
	FTransform PortalTransform = Portal->GetActorTransform();
	FTransform ActorTransform = Agent->GetActorTransform();

	UTeleportAgent* TeleportAgent = Agent->FindComponentByClass<UTeleportAgent>();

	FVector NewLocation = ConvertLocationToActorSpace(ActorTransform, PortalTransform, TargetTransform);
	FQuat NewRotation = ConvertRotationToActorSpace(ActorTransform, PortalTransform, TargetTransform);
	FTransform NewTransform(NewRotation, NewLocation, ActorTransform.GetScale3D());

	UE_LOG(LogTemp, Warning, TEXT("07 Updating Actor"));

	//ClonedActor->SetActorLocationAndRotation(NewLocation, NewRotation);
	ClonedActor->SetActorTransform(NewTransform);

	if (TeleportAgent->bIsPlayerController)
	{
		APortal2Character* PlayerCharacter = Cast<APortal2Character>(Agent);
		APortal2Character* ClonedPortal2Character = Cast<APortal2Character>(ClonedActor);

		FQuat NewRotationCam = ConvertRotationToActorSpace(PlayerController->PlayerCameraManager->GetTransform(), PortalTransform, TargetTransform);

		UCameraComponent* CameraComponent = ClonedPortal2Character->GetFirstPersonCameraComponent();

		USkeletalMeshComponent* ArmsComponent = ClonedPortal2Character->GetMesh1P();
		USkeletalMeshComponent* AgentArmsComponent = PlayerCharacter->GetMesh1P();

		if (CameraComponent && ArmsComponent)
		{
			UMyAnimInstance* AnimInstanceMain = Cast<UMyAnimInstance>(AgentArmsComponent->GetAnimInstance());
			UMyAnimInstance* AnimInstance = Cast<UMyAnimInstance>(ArmsComponent->GetAnimInstance());

			AnimInstance->bOverrideBools = false;
			AnimInstance->bIsMoving = AnimInstanceMain->bIsMoving;
			AnimInstance->bIsInAir = AnimInstanceMain->bIsInAir;
			AnimInstance->bHasRifle = AnimInstanceMain->bHasRifle;

			if (AnimInstanceMain->bFire == true)
			{
				TArray<AActor*> Weapons;
				TArray<AActor*> Weapons2;
				ClonedActor->GetAttachedActors(Weapons);
				Agent->GetAttachedActors(Weapons2);
				UTP_WeaponComponent* WeaponComp = Weapons[0]->FindComponentByClass<UTP_WeaponComponent>();
				UTP_WeaponComponent* WeaponComp2 = Weapons2[0]->FindComponentByClass<UTP_WeaponComponent>();
				WeaponComp->PlayFireAnimation(false);
				WeaponComp2->PlayFireAnimation(false);
			}

			ClonedPortal2Character->GetCharacterMovement()->Velocity = ConvertVelocityToActorSpace(PlayerCharacter->GetCharacterMovement()->Velocity, PortalTransform, TargetTransform);

			CameraComponent->SetWorldRotation(NewRotationCam);
		}
	}
	else if (TeleportAgent->IsA(APortal2Projectile::StaticClass()))
	{
		APortal2Projectile* Projectile = Cast<APortal2Projectile>(Agent);
		APortal2Projectile* ClonedProjectile = Cast<APortal2Projectile>(ClonedActor);
		ClonedProjectile->SetProjectileMovement(ConvertVelocityToActorSpace(Projectile->GetVelocity(), PortalTransform, TargetTransform));
	}
	else if (TeleportAgent->IsA(AActor::StaticClass()))
	{
		UPrimitiveComponent* PrimitiveComponent = Cast<UPrimitiveComponent>(ClonedActor->GetRootComponent());
		if (PrimitiveComponent)
		{
			PrimitiveComponent->SetPhysicsLinearVelocity(ConvertVelocityToActorSpace(Agent->GetVelocity(), PortalTransform, TargetTransform));
		}
	}
}

/**
 * Creates a new portal in the world at the specified location and rotation. 
 * Adds a portal reference to a UPortalSurface component for later referencing.
 * If it's the orange portal, links it with the blue portal,
 * and updates visuals. If it's the blue portal, does the reverse.
 * 
 * @param PortalCenter The center position of the portal.
 * @param PortalRotation The rotation of the portal.
 * @param bIsOrangePortal Indicates if the portal being created is the orange portal.
 * @param PortalSurfaceData The surface data associated with the portal.
 * @param Index The index of the portal in the list.
 */
void APortal3Manager::CreateNewPortal(FVector PortalCenter, FQuat PortalRotation, bool bIsOrangePortal, UPortalSurface* PortalSurfaceData, int32 Index)
{
	UE_LOG(LogTemp, Warning, TEXT("Creating Portal at: "), *PortalCenter.ToString());

	FQuat RotationCorrection = FQuat::MakeFromEuler(FVector(90.f, -90.f, 0.f));
	PortalRotation *= RotationCorrection;
	APortalV3* NewPortal = Cast<APortalV3>(GetWorld()->SpawnActor<AActor>(ABP_PortalV2, PortalCenter, PortalRotation.Rotator()));

	if (NewPortal == nullptr)
	{
		return;
	}

	if (bIsOrangePortal)
	{
		OrangePortal = NewPortal;
		OrangePortal->SetSurfaceData(Index, PortalSurfaceData);
		OrangePortal->SetPortalColor(FVector(50, 10, 0));

		if (OrangePortal != nullptr && BluePortal != nullptr)
		{
			OrangePortal->LinkedPortal = BluePortal;
			BluePortal->LinkedPortal = OrangePortal;
			if (BluePortal->PortalSurface)
			{
				UE_LOG(LogTemp, Warning, TEXT("No Surface Attached to Blue?!?"));
			}
			if (OrangePortal->PortalSurface)
			{
				UE_LOG(LogTemp, Warning, TEXT("No Surface Attached to Orange?!?"));
			}
			BluePortal->PortalSurface->RebuildCollisionMesh();
			OrangePortal->PortalSurface->RebuildCollisionMesh();
		}	
	}
	else
	{
		BluePortal = NewPortal;
		BluePortal->SetSurfaceData(Index, PortalSurfaceData);
		BluePortal->SetPortalColor(FVector(0, 10, 50));

		if (OrangePortal != nullptr && BluePortal != nullptr)
		{
			BluePortal->LinkedPortal = OrangePortal;
			OrangePortal->LinkedPortal = BluePortal;
			if (BluePortal->PortalSurface)
			{
				UE_LOG(LogTemp, Warning, TEXT("No Surface Attached to Blue?!?"));
			}
			if (OrangePortal->PortalSurface)
			{
				UE_LOG(LogTemp, Warning, TEXT("No Surface Attached to Orange?!?"));
			}
			BluePortal->PortalSurface->RebuildCollisionMesh();
			OrangePortal->PortalSurface->RebuildCollisionMesh();
			UE_LOG(LogTemp, Warning, TEXT("Staying Alive - Staying Alive"))
		}
	}

	NewPortal->bIsOrangePortal = bIsOrangePortal;

	PortalList = FindAllActorsInWorld<APortalV3>(GetWorld(), ABP_PortalV2);

	UpdateViewportSize(NewPortal);
}

/**
 * Destroys the existing orange or blue portal based on the specified flag.
 * Updates the PortalList array after destruction.
 *
 * @param bIsOrangePortal True if destroying the orange portal, false for the blue portal.
 */
void APortal3Manager::DestroyOldPortal(bool bIsOrangePortal)
{
	if (bIsOrangePortal)
	{
		if (OrangePortal != nullptr)
		{
			OrangePortal->PortalDestroySelf();
			OrangePortal = nullptr;
		}
	}
	else
	{
		if (BluePortal != nullptr)
		{
			BluePortal->PortalDestroySelf();
			BluePortal = nullptr;
		}
	}
	PortalList = FindAllActorsInWorld<APortalV3>(GetWorld(), ABP_PortalV2);
}

/**
 * Checks if the portal needs an update based on the current camera and portal transforms.
 *
 * This function compares the provided current transforms of the portal to check wether or not the portal texture should be rendered this frame.
 *
 * @param Portal The portal actor being checked.
 * @param PortalTransform The current transform of the portal.
 * @param CameraTransform The current transform of the camera.
 * @return true if the portal needs an update, false otherwise.
 */
bool APortal3Manager::CheckPortalNeedsUpdate(APortalV3* Portal, FTransform Reference, FTransform Camera)
{
	if (PlayerPortalDistance(Reference, Camera))
	{
		return true;
	}

	FQuat RotationQuat = FQuat(FRotator(0.f, 45.f, 0.f));
	FQuat RotationQuatInverse = FQuat(FRotator(0.f, -45.f, 0.f));

	if (CheckActorInFront(Reference, Camera) && CheckActorInFront(FTransform(Camera.GetRotation() * RotationQuat, Camera.GetLocation(), Camera.GetScale3D()), Reference) && CheckActorInFront(FTransform(Camera.GetRotation() * RotationQuatInverse, Camera.GetLocation(), Camera.GetScale3D()), Reference))
	{
		TArray<FVector> PortalBounds = Portal->GetPortalBounds();
		if (CheckPlayerPortalLineOfSigth(PortalBounds, Camera, Portal))
		{
			return true;
		}
	}
	return false;
}

/**
 * Checks if the player is within a specified distance threshold from a reference point. 
 * Prevents problems with updating the portal textures when the player moves through.
 *
 * @param Reference The transform representing the reference point.
 * @param Camera The transform representing the player's camera position.
 * @return True if the player is within 100 units of the reference point, false otherwise.
 */
bool APortal3Manager::PlayerPortalDistance(FTransform Reference, FTransform Camera)
{
	float Distance = FVector::Distance(Camera.GetLocation(), Reference.GetLocation());
	if (Distance <= 100)
	{
		return true;
	}
	return false;
}

/**
 * Checks if the camera or actor transform is in front of a reference plane defined by a transform. 
 *
 * @param Reference The transform representing the reference point and orientation of the plane.
 * @param Camera The transform representing the camera or actor position to check.
 * @return True if the camera or actor position is in front of the reference plane, false otherwise.
 */
bool APortal3Manager::CheckActorInFront(FTransform Reference, FTransform Camera)
{
	FPlane PortalPlane = FPlane(Reference.GetLocation(), Reference.GetRotation().GetForwardVector());
	float PortalDot = PortalPlane.PlaneDot(Camera.GetLocation());

	if (PortalDot < 0)
	{
		return false;
	}
	return true;
}

/**
 * Checks if the player's camera has a clear line of sight to any corner of a portal, using raycasting.
 *
 * @param PortalBounds Array of corner points defining the bounding volume of the portal in world space.
 * @param Camera Transform representing the player's camera position and orientation.
 * @param Portal Pointer to the portal actor to exclude from the raycast checks (to avoid self-intersection).
 * @return True if there is a clear line of sight from the camera to any portal corner, false otherwise.
 */
bool APortal3Manager::CheckPlayerPortalLineOfSigth(TArray<FVector> PortalBounds, FTransform Camera, AActor* Portal)
{
	for (const FVector& Corner : PortalBounds)
	{
		//DrawDebugSphere(GetWorld(), Corner, 20, 12, FColor::Green, false, -1.0f, 0, 1);
		if (RaycastClear(Camera.GetLocation(), Corner, Portal))
		{
			return true;
		}
	}
	return false;
}

/**
 * Performs a line trace to check if there are any obstructions between two points (Start and End).
 *
 * @param Start The starting point of the raycast.
 * @param End The end point of the raycast.
 * @param Portal Pointer to the portal actor to exclude from the raycast check (to avoid self-intersection).
 * @return True if there are no obstructions between Start and End, or if the only obstruction is the Portal itself. False otherwise.
 */
bool APortal3Manager::RaycastClear(const FVector& Start, const FVector& End, AActor* Portal)
{
	FHitResult HitResult;
	FCollisionQueryParams CollisionParams;

	bool bHit = GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_Visibility, CollisionParams);

	return !bHit || HitResult.GetActor() == Portal;
}

/**
* Public function to assign a bIsCloned status to TeleportAgents. This status is based on what moment during 
* the tick function this function is called. It is a rusty solution to spawning blueprints with variables attached.
* 
* @return True is the actor was cloned instead of original.
*/
bool APortal3Manager::GetCloneStatus()
{
	return bCloneState;
}

/**
 * Converts a location from one actor's space (Camera) to another actor's space (Target) based on a reference actor (Reference).
 * In the portal project, this is used to both teleport actors and update screencapture camera positions.
 *
 * @param Camera The transform of the camera or source actor from which the location is being converted.
 * @param Reference The transform of the reference actor to which the Camera's location is relative.
 * @param Target The transform of the target actor to which the converted location is relative.
 * @return The location vector converted to the space of the Target actor.
 */
FVector APortal3Manager::ConvertLocationToActorSpace(FTransform Camera, FTransform Reference, FTransform Target)
{
	FVector Direction = Camera.GetLocation() - Reference.GetLocation();

	FVector Dots;
	Dots.X = FVector::DotProduct(Direction, Reference.GetRotation().GetForwardVector());
	Dots.Y = FVector::DotProduct(Direction, Reference.GetRotation().GetRightVector());
	Dots.Z = FVector::DotProduct(Direction, Reference.GetRotation().GetUpVector());

	FVector NewDirection = Dots.X * -Target.GetRotation().GetForwardVector() + Dots.Y * -Target.GetRotation().GetRightVector() + Dots.Z * Target.GetRotation().GetUpVector();

	return Target.GetLocation() + NewDirection;
}

/**
 * Converts a rotation from one actor's space (Camera) to another actor's space (Target) based on a reference actor (Reference).
 * In the portal project, this is used to both rotate teleported actors and update screencapture camera rotations.
 *
 * @param Camera The transform of the camera or source actor from which the rotation is being converted.
 * @param Reference The transform of the reference actor to which the Camera's rotation is relative.
 * @param Target The transform of the target actor to which the converted rotation is relative.
 * @return The rotation quaternion converted to the space of the Target actor.
 */
FQuat APortal3Manager::ConvertRotationToActorSpace(FTransform Camera, FTransform Reference, FTransform Target)
{
	FQuat LocalQuat = Reference.GetRotation().Inverse() * Camera.GetRotation();
	LocalQuat = FQuat(FVector::UpVector, PI) * LocalQuat;

	return Target.GetRotation() * LocalQuat;
}

/**
 * Converts a velocity vector from one actor's space (Reference) to another actor's space (Target).
 * In the portal project, this is used to update the velocity vector of teleported actors.
 *
 * @param Object The velocity vector of the object in the Reference actor's space.
 * @param Reference The transform of the reference actor whose space the velocity is relative to.
 * @param Target The transform of the target actor to which the converted velocity should be relative.
 * @return The velocity vector converted to the space of the Target actor.
 */
FVector APortal3Manager::ConvertVelocityToActorSpace(FVector Object, FTransform Reference, FTransform Target)
{
	FVector Direction = Object;

	FVector Dots;
	Dots.X = FVector::DotProduct(Direction, Reference.GetRotation().GetForwardVector());
	Dots.Y = FVector::DotProduct(Direction, Reference.GetRotation().GetRightVector());
	Dots.Z = FVector::DotProduct(Direction, Reference.GetRotation().GetUpVector());

	FVector NewVelocity = Dots.X * -Target.GetRotation().GetForwardVector() + Dots.Y * -Target.GetRotation().GetRightVector() + Dots.Z * Target.GetRotation().GetUpVector();

	return NewVelocity;
}

/**
 * Retrieves the camera projection matrix based on the current view or projection settings.
 *
 * @param CameraManagerIn The player camera manager instance that manages the camera settings.
 * @param bIsView Flag indicating whether to retrieve the view matrix (true) or projection matrix (false).
 * @return The requested matrix (ViewProjectionMatrix if bIsView is true, ProjectionMatrix if false).
 */
FMatrix APortal3Manager::GetCameraProjectionMatrix(APlayerCameraManager* CameraManagerIn, bool bIsView)
{
	FMinimalViewInfo CameraView = CameraManagerIn->GetCameraCacheView();

	FMatrix ViewMatrix;
	FMatrix ProjectionMatrix;
	FMatrix ViewProjectionMatrix;

	UGameplayStatics::GetViewProjectionMatrix(CameraView, ViewMatrix, ProjectionMatrix, ViewProjectionMatrix);
	if (bIsView)
	{
		return ViewProjectionMatrix;
	}
	else
	{
		return ProjectionMatrix;
	}
}

/**
 * Teleports the specified actor through the given portal.
 *
 * @param Agent The actor to teleport.
 * @param Portal The portal through which the actor will be teleported.
 */
void APortal3Manager::TeleportActor(AActor* Agent, APortalV3* Portal)
{
	UTeleportAgent* TeleportAgent = Agent->FindComponentByClass<UTeleportAgent>();
	
	FTransform PortalTransform = Portal->GetActorTransform();
	FTransform TargetTransform = Portal->LinkedPortal->GetActorTransform();
	FTransform ActorTransform = Agent->GetActorTransform();

	FVector NewLocation = ConvertLocationToActorSpace(ActorTransform, PortalTransform, TargetTransform);
	FQuat NewRotation = ConvertRotationToActorSpace(ActorTransform, PortalTransform, TargetTransform);
	FVector NewVelocity = ConvertVelocityToActorSpace(Agent->GetVelocity() ,PortalTransform, TargetTransform);

	USceneComponent* RootComponent2 = Agent->GetRootComponent();
	UPrimitiveComponent* PrimitiveRootComponent = Cast<UPrimitiveComponent>(RootComponent2);
	PrimitiveRootComponent->SetPhysicsLinearVelocity(NewVelocity);

	FHitResult HitResult;

	if (TeleportAgent->bIsPlayerController)
	{
		Agent->SetActorLocation(NewLocation, false, &HitResult, ETeleportType::TeleportPhysics);

		// causes issues with horizontal to vertical portal rotations, but fixes stutter in same plane portals (ONLY FOR CHARACTER)?!?
		Agent->SetActorRotation(FRotator(NewRotation.Rotator().Pitch, NewRotation.Rotator().Yaw, NewRotation.Rotator().Roll));
		ACharacter* Char = Cast<ACharacter>(Agent);
		Char->bUseControllerRotationYaw = false;
		Char->bUseControllerRotationRoll = false;
		

		FRotator NewRotation2 = PlayerController->GetControlRotation();
		FQuat LocalQuat = PortalTransform.GetRotation().Inverse() * NewRotation2.Quaternion();
		LocalQuat = FQuat(FVector::UpVector, PI) * LocalQuat;
		LocalQuat = TargetTransform.GetRotation() * LocalQuat;
		
		
		PlayerController->SetControlRotation(FRotator(LocalQuat.Rotator().Pitch, LocalQuat.Rotator().Yaw, LocalQuat.Rotator().Roll));
		PlayerController->GetCharacter()->GetCharacterMovement()->Velocity = NewVelocity;

		//Agent->SetActorRotation(FRotator(NewRotation.Rotator().Pitch, NewRotation.Rotator().Yaw, NewRotation.Rotator().Roll));
	}
	else if (Agent->IsA(APortal2Projectile::StaticClass()))
	{
		Agent->SetActorLocation(NewLocation, false, &HitResult, ETeleportType::TeleportPhysics);
		Agent->SetActorRotation(NewRotation);
		APortal2Projectile* Projectile = Cast<APortal2Projectile>(Agent);
		Projectile->SetProjectileMovement(NewVelocity);
	}
	else
	{
		Agent->SetActorLocation(NewLocation, false, &HitResult, ETeleportType::TeleportPhysics);
		Agent->SetActorRotation(NewRotation);
	}
}

/**
 * Maps all actors in the world that have a UTeleportAgent component, excluding those with bDoNotTeleport set to true.
 *
 * @param World The world context to search for actors.
 * @param OutActors The output map where mapped actors will be stored (Key: Actor with UTeleportAgent component, Value: Actor itself).
 */
void APortal3Manager::MapAllActorsWithComponent(UWorld* World, TMap<AActor*, AActor*>& OutActors)
{
	if (!World)
	{
		return;
	}

	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Actor = *It;
		if (Actor->FindComponentByClass<UTeleportAgent>())
		{
			if (!Actor->FindComponentByClass<UTeleportAgent>()->bDoNotTeleport)
			{
				OutActors.FindOrAdd(Actor, Actor);
			}
		}
	}
}

/**
 * Finds the cloned actor corresponding to the given Agent and Portal.
 *
 * @param Agent The original actor that was cloned.
 * @param Portal The portal through which the actor was cloned.
 * @return The cloned actor corresponding to Agent and Portal, or nullptr if not found.
 */
AActor* APortal3Manager::FindClonedActor(AActor* Agent, APortalV3* Portal)
{
	FAgentPortalKey Key(Agent, Portal);
	AActor** ClonedActorPtr = ClonedActors.Find(Key);
	return ClonedActorPtr ? *ClonedActorPtr : nullptr;
}

/**
 * Stores the cloned actor corresponding to the given Agent and Portal in the ClonedActors map.
 *
 * @param Agent The original actor that was cloned.
 * @param Portal The portal through which the actor was cloned.
 * @param ClonedActor The cloned actor to store.
 */
void APortal3Manager::StoreClonedActor(AActor* Agent, APortalV3* Portal, AActor* ClonedActor)
{
	FAgentPortalKey Key(Agent, Portal);
	ClonedActors.Add(Key, ClonedActor);
}

/**
 * Removes the cloned actor associated with the given Agent and Portal from the ClonedActors map and destroys it.
 *
 * @param Agent The original actor whose clone needs to be removed.
 * @param Portal The portal associated with the cloned actor to be removed.
 */
void APortal3Manager::RemoveClonedActor(AActor* Agent, APortalV3* Portal)
{
	FAgentPortalKey Key(Agent, Portal);
	AActor* ClonedActor = FindClonedActor(Agent, Portal);
	if (ClonedActor != nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("08 Removing Cloned Actor"));
		TArray<AActor*> AttachedActors;
		ClonedActor->GetAttachedActors(AttachedActors);

		for (AActor* AttachedActor : AttachedActors)
		{
			AttachedActor->Destroy();
		}

		ClonedActors.Remove(Key);
		ClonedActor->Destroy();
	}
}

/**
 * Function that can be called to add a actor with the UTeleportAgent component to the teleportable actors map.
 */
void APortal3Manager::HandleActorSpawned(AActor* Actor)
{
	if (Actor->FindComponentByClass<UTeleportAgent>())
	{
		TeleportAgents.FindOrAdd(Actor, Actor);
	}
}

/**
 * Function that can be called to remove a actor with the UTeleportAgent component to the teleportable actors map.
 */
void APortal3Manager::HandleActorDestroyed(AActor* Actor)
{
	if (Actor->FindComponentByClass<UTeleportAgent>())
	{
		UE_LOG(LogTemp, Warning, TEXT("Removed Actor"));
		TeleportAgents.Remove(Actor);
	}
}
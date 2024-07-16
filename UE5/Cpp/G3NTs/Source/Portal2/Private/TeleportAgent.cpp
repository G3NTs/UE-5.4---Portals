// Fill out your copyright notice in the Description page of Project Settings.

#include "TeleportAgent.h"
#include "C:/Users/G3NTs/Documents/Unreal Projects/Portal2/Source/Portal2/TP_WeaponComponent.h"
#include "Materials/MaterialInterface.h"
#include "Engine/SkinnedAssetCommon.h"
#include "Portal3Manager.h"

UTeleportAgent::UTeleportAgent()
{
	PrimaryComponentTick.bCanEverTick = false;

	bIsCloned = false;
	bDoNotTeleport = false;
}

void UTeleportAgent::BeginPlay()
{
	Super::BeginPlay();

	if (AActor* Owner = GetOwner())
	{
		UPrimitiveComponent* RootComponent = Cast<UPrimitiveComponent>(GetOwner()->GetRootComponent());
		CollisionProfileName = RootComponent->GetCollisionProfileName();

		if (UWorld* World = Owner->GetWorld())
		{
			/**
			 * Dynamic material instances are assigned to the teleportable actors in order to get the clip plane working.
			 * Without the dynamic material instances, the clip plane would not update. 
			 * Two mesh component types are assigned the correct materials. One is the Skeletal mesh component, the other the static one. 
			 * The code is made to work with multiple materials per mesh.
			 */ 
			if (UStaticMeshComponent* MeshComponent = Owner->FindComponentByClass<UStaticMeshComponent>())
			{
				int32 MaterialCount = MeshComponent->GetNumMaterials();
				DynamicMaterialInstances.SetNum(MaterialCount);
				for (int32 i = 0; i < MaterialCount; ++i)
				{
					UMaterialInterface* Material = MeshComponent->GetMaterial(i);
					if (Material)
					{
						DynamicMaterialInstances[i] = UMaterialInstanceDynamic::Create(Material, this);
						MeshComponent->SetMaterial(i, DynamicMaterialInstances[i]);
					}
				}
			}
			TArray<USkeletalMeshComponent*> SkeletalMeshComponents;
			Owner->GetComponents<USkeletalMeshComponent>(SkeletalMeshComponents);
			for (USkeletalMeshComponent* SkeletalComponent : SkeletalMeshComponents)
			{
				for (int32 i = 0; i < SkeletalComponent->GetNumMaterials(); ++i)
				{
					MaterialInterface = SkeletalComponent->GetMaterial(i);
					if (MaterialInterface)
					{
						UMaterialInstanceDynamic* DynamicMaterialInstance = UMaterialInstanceDynamic::Create(MaterialInterface, this);
						SkeletalComponent->SetMaterial(i, DynamicMaterialInstance);
						DynamicMaterialInstances.Add(DynamicMaterialInstance);
					}
				}
			}

			/**
			 * This bit of the code manages initializing the variables of the teleport agent. 
			 * It prevents cloned teleport actors to be cloned again.
			 */
			for (TActorIterator<APortal3Manager> It(World); It; ++It)
			{
				APortal3Manager* Manager = *It;
				if (Manager)
				{
					bIsCloned = Manager->GetCloneStatus();
					if (bIsCloned || bDoNotTeleport)
					{
						return;
					}
					UE_LOG(LogTemp, Warning, TEXT("Added Actor"));
					Manager->HandleActorSpawned(Owner);
					return;
				}
			}
		}
	}
}

void UTeleportAgent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	if (bIsCloned)
	{
		return;
	}

	if (AActor* Owner = GetOwner())
	{
		if (UWorld* World = Owner->GetWorld())
		{
			for (TActorIterator<APortal3Manager> It(World); It; ++It)
			{
				APortal3Manager* Manager = *It;
				if (Manager)
				{
					if (bIsCloned || bDoNotTeleport)
					{
						return;
					}
					UE_LOG(LogTemp, Warning, TEXT("Removed Actor"));
					Manager->HandleActorDestroyed(Owner);
					return;
				}
			}
		}
	}
}

/**
 * Sets the teleportation status for a given actor
 *
 * @param Actor The actor whose teleport status is being set
 * @param bCanTeleport The new teleport status
 */
void UTeleportAgent::SetTeleportStatus(AActor* Actor, bool bCanTeleport)
{
	bool& ExistingStatus = TeleportStatus.FindOrAdd(Actor, bCanTeleport);
	ExistingStatus = bCanTeleport;
}

/**
 * Sets the clipping plane with a specified location and forward vector
 *
 * @param InLocation The location to set the clip plane
 * @param InForwardVector The forward vector for the clip plane
 */
void UTeleportAgent::SetClipPlane(FVector InLocation, FVector InForwardVector)
{
	for (UMaterialInstanceDynamic* DynamicMaterialInstance : DynamicMaterialInstances)
	{
		DynamicMaterialInstance->SetVectorParameterValue(TEXT("Position"), InLocation - InForwardVector);
		DynamicMaterialInstance->SetVectorParameterValue(TEXT("Normal"), -InForwardVector);
		DynamicMaterialInstance->SetScalarParameterValue(TEXT("bClipPlaneEnabled"), 1.0f);
	}

	AActor* Owner = GetOwner();
	if (!Owner) return;

	TArray<AActor*> AttachedActors;
	Owner->GetAttachedActors(AttachedActors);

	for (AActor* AttachedActor : AttachedActors)
	{
		// Check if the attached actor has a TeleportAgentComponent
		UTeleportAgent* AttachedTeleportAgent = AttachedActor->FindComponentByClass<UTeleportAgent>();
		if (AttachedTeleportAgent && AttachedTeleportAgent != this)
		{
			// Apply the same clip plane settings to the attached component
			for (UMaterialInstanceDynamic* DynamicMaterialInstance : AttachedTeleportAgent->DynamicMaterialInstances)
			{
				DynamicMaterialInstance->SetVectorParameterValue(TEXT("Position"), InLocation - InForwardVector);
				DynamicMaterialInstance->SetVectorParameterValue(TEXT("Normal"), -InForwardVector);
				DynamicMaterialInstance->SetScalarParameterValue(TEXT("bClipPlaneEnabled"), 1.0f);
			}
		}
	}
}

/**
 * Disables the clipping plane
 */
void UTeleportAgent::DisableClipPlane()
{
	for (UMaterialInstanceDynamic* DynamicMaterialInstance : DynamicMaterialInstances)
	{
		DynamicMaterialInstance->SetScalarParameterValue(TEXT("bClipPlaneEnabled"), 0.0f);
	}
	AActor* Owner = GetOwner();
	if (!Owner) return;

	TArray<AActor*> AttachedActors;
	Owner->GetAttachedActors(AttachedActors);

	for (AActor* AttachedActor : AttachedActors)
	{
		// Check if the attached actor has a TeleportAgentComponent
		UTeleportAgent* AttachedTeleportAgent = AttachedActor->FindComponentByClass<UTeleportAgent>();
		if (AttachedTeleportAgent && AttachedTeleportAgent != this)
		{
			// Apply the same clip plane settings to the attached component
			for (UMaterialInstanceDynamic* DynamicMaterialInstance : AttachedTeleportAgent->DynamicMaterialInstances)
			{
				DynamicMaterialInstance->SetScalarParameterValue(TEXT("bClipPlaneEnabled"), 0.0f);
			}
		}
	}
}

/**
 * Gets the teleportation status for a given actor
 *
 * @param Actor The actor whose teleport status is being queried
 * @return The teleport status of the actor
 */
bool UTeleportAgent::GetTeleportStatus(AActor* Actor)
{
	const bool* TeleportStatusPtr = TeleportStatus.Find(Actor);
	if (TeleportStatusPtr)
	{
		return *TeleportStatusPtr;
	}
	return false;
}

/**
 * Changes the collision settings for the agent
 *
 * @param bCollisionEnabled Whether to enable or disable collision
 */
void UTeleportAgent::ChangeAgentCollision(bool bCollisionEnabled)
{
	if (!bCollisionEnabled)
	{
		UPrimitiveComponent* RootComponent = Cast<UPrimitiveComponent>(GetOwner()->GetRootComponent());
		RootComponent->SetCollisionProfileName(TEXT("PortalAgent"));
	}
}

/**
 * Resets the collision settings for the agent to the original settings
 */
void UTeleportAgent::ResetAgentCollision()
{
	UPrimitiveComponent* RootComponent = Cast<UPrimitiveComponent>(GetOwner()->GetRootComponent());
	RootComponent->SetCollisionProfileName(CollisionProfileName);
}


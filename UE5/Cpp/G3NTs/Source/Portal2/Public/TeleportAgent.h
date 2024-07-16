// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TeleportAgent.generated.h"

class APortal3Manager;
class UTP_WeaponComponent;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class PORTAL2_API UTeleportAgent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UTeleportAgent();

private:
	UPROPERTY(VisibleAnywhere, Category = "G3NTs|Portal")
	TMap<AActor*, bool> TeleportStatus;

	/** Dynamic material instances used for the clipping plane effect */
	UPROPERTY(VisibleAnywhere, Category = "G3NTs|Portal")
	TArray<UMaterialInstanceDynamic*> DynamicMaterialInstances;

	/** Material interface used by the actor */
	UPROPERTY(VisibleAnywhere, Category = "G3NTs|Portal")
	UMaterialInterface* MaterialInterface;

public:
	/** Flag indicating if the actor is controlled by a player */
	UPROPERTY(EditAnywhere, Category = "G3NTs|Portal")
	bool bIsPlayerController;

	/** Flag indicating if the actor is a cloned entity */
	UPROPERTY(EditAnywhere, Category = "G3NTs|Portal")
	bool bIsCloned;

	/** Flag indicating if the actor should not be teleported */
	UPROPERTY(EditAnywhere, Category = "G3NTs|Portal")
	bool bDoNotTeleport;

	/** Flag indicating if the actor is attached to another actor */
	UPROPERTY(EditAnywhere, Category = "G3NTs|Portal")
	bool bIsAttached;

	/** The collision profile name before any changes were made */
	FName CollisionProfileName;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	/**
	 * Sets the teleportation status for a given actor
	 *
	 * @param Actor The actor whose teleport status is being set
	 * @param bCanTeleport The new teleport status
	 */
	UFUNCTION(BlueprintCallable, Category = "G3NTs|Portal")
	void SetTeleportStatus(AActor* Actor, bool bCanTeleport);

	/**
	 * Sets the clipping plane with a specified location and forward vector
	 *
	 * @param InLocation The location to set the clip plane
	 * @param InForwardVector The forward vector for the clip plane
	 */
	UFUNCTION(BlueprintCallable, Category = "G3NTs|Portal")
	void SetClipPlane(FVector InLocation, FVector InForwardVector);

	/**
	 * Disables the clipping plane
	 */
	UFUNCTION(BlueprintCallable, Category = "G3NTs|Portal")
	void DisableClipPlane();

	/**
	 * Gets the teleportation status for a given actor
	 *
	 * @param Actor The actor whose teleport status is being queried
	 * @return The teleport status of the actor
	 */
	UFUNCTION(BlueprintCallable, Category = "G3NTs|Portal")
	bool GetTeleportStatus(AActor* Actor);

	/**
	 * Changes the collision settings for the agent
	 *
	 * @param bCollisionEnabled Whether to enable or disable collision
	 */
	void ChangeAgentCollision(bool bCollisionEnabled);

	/**
	 * Resets the collision settings for the agent to the original settings
	 */
	void ResetAgentCollision();
};

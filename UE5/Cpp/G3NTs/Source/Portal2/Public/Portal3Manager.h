// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "TeleportAgent.h"
#include "EngineUtils.h"

#include "PortalV3.h"
#include "DebugDisplay.h"

#include "GameFramework/PlayerController.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

#include "Portal3Manager.generated.h"

/**
* Forward declarations, to prevent circular dependecies. 
*/
class APortal2Projectile;
class ABP_PortalV2;
class UPortalSurface;

/**
 * Structure used to create a key of a combination between portal and teleportable actor. 
 * In other words, it creates a reference to a combination. This struct is used to find and store cloned actor references, 
 * so that a clone can be updated in accordance with the original actor. As well as to prevent duplicate clones. 
 */
USTRUCT(BlueprintType)
struct FAgentPortalKey
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly)
	AActor* Agent; // Pointer to the original agent actor

	UPROPERTY(BlueprintReadOnly)
	APortalV3* Portal; // Pointer to the portal associated with the cloned actor

	FAgentPortalKey() : Agent(nullptr), Portal(nullptr) {}

	FAgentPortalKey(AActor* InAgent, APortalV3* InPortal)
		: Agent(InAgent), Portal(InPortal)
	{}

	/**
	 * Comparison operator to check equality of FAgentPortalKey objects. 
	 *
	 * @param Other The other FAgentPortalKey object to compare with.
	 * @return True if both Agent and Portal pointers are equal, false otherwise.
	 */
	bool operator==(const FAgentPortalKey& Other) const
	{
		return Agent == Other.Agent && Portal == Other.Portal;
	}

	/**
	 * Hash function to generate a unique hash value for FAgentPortalKey objects. This way, the key per combination is unique.
	 *
	 * @param Key The FAgentPortalKey object for which to generate the hash.
	 * @return The hash value based on Agent and Portal pointers.
	 */
	friend uint32 GetTypeHash(const FAgentPortalKey& Key)
	{
		return GetTypeHash(Key.Agent) ^ GetTypeHash(Key.Portal);
	}
};

/**
 * Manager class responsible for handling portals and teleportation mechanics in the game.
 */
UCLASS()
class PORTAL2_API APortal3Manager : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	APortal3Manager();

private:
	FActorTickFunction SecondaryActorTick;
	bool bCloneState;

private:
	/**
	 * Important Maps Storing both the actors that can be teleported as well as the cloned actors.
	 */

	UPROPERTY(VisibleAnywhere)
	TMap<AActor*, AActor*> TeleportAgents;

	UPROPERTY(VisibleAnywhere)
	TMap<FAgentPortalKey, AActor*> ClonedActors;

	UPROPERTY(VisibleAnywhere)
	TArray<APortalV3*> PortalList;

	// Class that needs to be set before running the game!
	UPROPERTY(EditAnywhere)
	TSubclassOf<AActor> ABP_PortalV2;

public:
	UPROPERTY(EditAnywhere)
	APlayerController* PlayerController;

	// Actor pointers, for public access from other classes

	UPROPERTY(VisibleAnywhere)
	APortalV3* OrangePortal;

	UPROPERTY(VisibleAnywhere)
	APortalV3* BluePortal;

protected:
	virtual void BeginPlay() override;
	virtual void RegisterActorTickFunctions(bool bRegister) override;
	virtual void TickActor(float DeltaSeconds, ELevelTick TickType, FActorTickFunction& ThisTickFunction) override;

private:
	/*
	* StandAlone functions that do not require input arguments
	*/

	/**
	 * Updates the capture state of all portals in the PortalList.
	 *
	 * This function iterates through each portal in the PortalList and checks if the portal has a linked portal.
	 * If a linked portal is found, it calculates the necessary transforms and updates the portal's screen capture
	 * to reflect the current state of the camera relative to the portal and its linked portal.
	 */
	void UpdatePortals();

	/**
	 * Checks and manages teleportation for each agent in the TeleportAgents map.
	 * Iterates through each agent and evaluates its position relative to portals in the PortalList.
	 * Manages teleportation status, collision settings, and clip plane for each teleport agent.
	 */
	void TeleportActorsCheck();

	/**
	 * Iterates through all teleport agents and checks their teleport status for each portal.
	 * Clones or updates actors based on teleport status. Removes cloned actors if teleport
	 * status is false for a portal.
	 * Could possibly have been combined with Teleport Actors Check function!
	 */
	void CloneOrUpdateAllActors();

public:
	// Functions for the modifying the Teleportable Actors Map

	/**
	 * Function that can be called to add a actor with the UTeleportAgent component to the teleportable actors map.
	 */
	void HandleActorSpawned(AActor* Actor);

	/**
	 * Function that can be called to remove a actor with the UTeleportAgent component to the teleportable actors map.
	 */
	void HandleActorDestroyed(AActor* Actor);

	// Functions for the modifying the ClonedActorMap

	/**
	 * Finds the cloned actor corresponding to the given Agent and Portal.
	 *
	 * @param Agent The original actor that was cloned.
	 * @param Portal The portal through which the actor was cloned.
	 * @return The cloned actor corresponding to Agent and Portal, or nullptr if not found.
	 */
	AActor* FindClonedActor(AActor* Agent, APortalV3* Portal);
	
	/**
	 * Stores the cloned actor corresponding to the given Agent and Portal in the ClonedActors map.
	 *
	 * @param Agent The original actor that was cloned.
	 * @param Portal The portal through which the actor was cloned.
	 * @param ClonedActor The cloned actor to store.
	 */
	void StoreClonedActor(AActor* Agent, APortalV3* Portal, AActor* ClonedActor);

	/**
	 * Removes the cloned actor associated with the given Agent and Portal from the ClonedActors map and destroys it.
	 *
	 * @param Agent The original actor whose clone needs to be removed.
	 * @param Portal The portal associated with the cloned actor to be removed.
	 */
	void RemoveClonedActor(AActor* Agent, APortalV3* Portal);

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
	void CreateNewPortal(FVector PortalCenter, FQuat PortalRotation, bool bIsOrangePortal, UPortalSurface* PortalSurfaceData, int32 Index);
	
	/**
	 * Destroys the existing orange or blue portal based on the specified flag.
	 * Updates the PortalList array after destruction.
	 *
	 * @param bIsOrangePortal True if destroying the orange portal, false for the blue portal.
	 */
	void DestroyOldPortal(bool bIsOrangePortal);

	/**
	 * Public function to assign a bIsCloned status to TeleportAgents. This status is based on what moment during
	 * the tick function this function is called. It is a rusty solution to spawning blueprints with variables attached.
	 *
	 * @return True is the actor was cloned instead of original.
	 */
	bool GetCloneStatus();

private:
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
	void UpdatePortalCapture(APortalV3* Portal, FTransform Reference, FTransform Target, FTransform Camera);

	/**
	 * Deprecated! No longer used in the final version of the code
	 */
	void UpdateDebugDisplay(ADebugDisplay* DebugDisplayActor);

	/**
	 * Resets the control and actor rotations of the player character smoothly using spherical linear interpolation (SLERP).
	 * This function corrects the player's orientation after passing through portals that may have altered their rotation.
	 *
	 * @param DeltaTime The time elapsed since the last frame.
	 */
	void ResetRotationControllerSlerp(float DeltaTime);

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
	void CloneOrUpdateActor(AActor* Agent, APortalV3* Portal);

	/**
	 * Clones the specified actor through the given portal.
	 * Depending on the type of actor (player character, projectile, or static mesh),
	 * it handles cloning and transformation appropriately.
	 *
	 * @param Agent The actor to be cloned.
	 * @param Portal The portal through which the actor will be cloned.
	 */
	void CloneActor(AActor* Agent, APortalV3* Portal);

	/**
	 * Updates the cloned actor's position, rotation, and other properties based on the agent's state
	 * and the portal through which it was cloned. Depending on the type of actor (player character, projectile, or static mesh),
	 * it handles cloning and transformation appropriately.
	 *
	 * @param Agent The original actor whose clone is being updated.
	 * @param ClonedActor The cloned actor that needs to be updated.
	 * @param Portal The portal through which the actor was cloned.
	 */
	void UpdateClonedActor(AActor* Agent, AActor* ClonedActor, APortalV3* Portal);
	
	/**
	 * Teleports the specified actor through the given portal.
	 *
	 * @param Agent The actor to teleport.
	 * @param Portal The portal through which the actor will be teleported.
	 */
	void TeleportActor(AActor* Agent, APortalV3* Portal);

	/**
	 * Maps all actors in the world that have a UTeleportAgent component, excluding those with bDoNotTeleport set to true.
	 *
	 * @param World The world context to search for actors.
	 * @param OutActors The output map where mapped actors will be stored (Key: Actor with UTeleportAgent component, Value: Actor itself).
	 */
	void MapAllActorsWithComponent(UWorld* World, TMap<AActor*, AActor*>& OutActors);
	

	/*
	* Bool functions
	*/

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
	bool CheckPortalNeedsUpdate(APortalV3* Portal, FTransform Reference, FTransform Camera);

	/**
	 * Checks if the player is within a specified distance threshold from a reference point.
	 * Prevents problems with updating the portal textures when the player moves through.
	 *
	 * @param Reference The transform representing the reference point.
	 * @param Camera The transform representing the player's camera position.
	 * @return True if the player is within 100 units of the reference point, false otherwise.
	 */
	bool PlayerPortalDistance(FTransform Reference, FTransform Camera);

	/**
	 * Checks if the camera or actor transform is in front of a reference plane defined by a transform.
	 *
	 * @param Reference The transform representing the reference point and orientation of the plane.
	 * @param Camera The transform representing the camera or actor position to check.
	 * @return True if the camera or actor position is in front of the reference plane, false otherwise.
	 */
	bool CheckActorInFront(FTransform Reference, FTransform Camera);

	/**
	 * Checks if the player's camera has a clear line of sight to any corner of a portal, using raycasting.
	 *
	 * @param PortalBounds Array of corner points defining the bounding volume of the portal in world space.
	 * @param Camera Transform representing the player's camera position and orientation.
	 * @param Portal Pointer to the portal actor to exclude from the raycast checks (to avoid self-intersection).
	 * @return True if there is a clear line of sight from the camera to any portal corner, false otherwise.
	 */
	bool CheckPlayerPortalLineOfSigth(TArray<FVector> PortalBounds, FTransform Camera, AActor* Portal);

	/**
	 * Performs a line trace to check if there are any obstructions between two points (Start and End).
	 *
	 * @param Start The starting point of the raycast.
	 * @param End The end point of the raycast.
	 * @param Portal Pointer to the portal actor to exclude from the raycast check (to avoid self-intersection).
	 * @return True if there are no obstructions between Start and End, or if the only obstruction is the Portal itself. False otherwise.
	 */
	bool RaycastClear(const FVector& Start, const FVector& End, AActor* Portal);

	/**
	 * Updates the viewport size and applies it to all portals or a single one.
	 *
	 * @param APortalV3 Reference, if provided, the function will only update the specific portal reference.
	 * @return True if the viewport size was successfully retrieved, false if default size was used.
	 */
	bool UpdateViewportSize(APortalV3* Portal = nullptr);

	/*
	* Return Functions
	*/

	/**
	 * Converts a location from one actor's space (Camera) to another actor's space (Target) based on a reference actor (Reference).
	 * In the portal project, this is used to both teleport actors and update screencapture camera positions.
	 *
	 * @param Camera The transform of the camera or source actor from which the location is being converted.
	 * @param Reference The transform of the reference actor to which the Camera's location is relative.
	 * @param Target The transform of the target actor to which the converted location is relative.
	 * @return The location vector converted to the space of the Target actor.
	 */
	FVector ConvertLocationToActorSpace(FTransform Camera, FTransform Reference, FTransform Target);

	/**
	 * Converts a rotation from one actor's space (Camera) to another actor's space (Target) based on a reference actor (Reference).
	 * In the portal project, this is used to both rotate teleported actors and update screencapture camera rotations.
	 *
	 * @param Camera The transform of the camera or source actor from which the rotation is being converted.
	 * @param Reference The transform of the reference actor to which the Camera's rotation is relative.
	 * @param Target The transform of the target actor to which the converted rotation is relative.
	 * @return The rotation quaternion converted to the space of the Target actor.
	 */
	FQuat ConvertRotationToActorSpace(FTransform Camera, FTransform Reference, FTransform Target);

	/**
	 * Converts a velocity vector from one actor's space (Reference) to another actor's space (Target).
	 * In the portal project, this is used to update the velocity vector of teleported actors.
	 *
	 * @param Object The velocity vector of the object in the Reference actor's space.
	 * @param Reference The transform of the reference actor whose space the velocity is relative to.
	 * @param Target The transform of the target actor to which the converted velocity should be relative.
	 * @return The velocity vector converted to the space of the Target actor.
	 */
	FVector ConvertVelocityToActorSpace(FVector Object, FTransform Reference, FTransform Target);

	/**
	 * Retrieves the camera projection matrix based on the current view or projection settings.
	 *
	 * @param CameraManagerIn The player camera manager instance that manages the camera settings.
	 * @param bIsView Flag indicating whether to retrieve the view matrix (true) or projection matrix (false).
	 * @return The requested matrix (ViewProjectionMatrix if bIsView is true, ProjectionMatrix if false).
	 */
	FMatrix GetCameraProjectionMatrix(APlayerCameraManager* CameraManagerIn, bool bIsView);

	/*
	* Template Functions || would do really well in a seperate function library
	*/

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
	template <typename SubClass>
	TArray<SubClass*> FindAllActorsInWorld(UWorld* World, TSubclassOf<AActor> BaseClass);
};
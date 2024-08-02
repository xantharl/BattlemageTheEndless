// Fill out your copyright notice in the Description page of Project Settings.


#include "WallRunAbility.h"

bool UWallRunAbility::WallRunContinuationRayCast(AActor* sourceActor, UStaticMeshComponent* mesh)
{
	// Raycast from vaultRaycastSocket straight forward to see if Object is in the way
	// TODO: Figure out if mesh can be found from actor
	FVector start = mesh->GetSocketLocation(FName("feetRaycastSocket"));
	// Cast a ray out in hit normal direction 100 units long
	FVector castVector = (FVector::XAxisVector * 100).RotateAngleAxis(WallRunHit.ImpactNormal.RotateAngleAxis(180.f, FVector::ZAxisVector).Rotation().Yaw, FVector::ZAxisVector);
	FVector end = start + castVector;

	// Perform the raycast
	FHitResult hit;
	FCollisionQueryParams params;
	FCollisionObjectQueryParams objectParams;
	params.AddIgnoredActor(sourceActor);
	GetWorld()->LineTraceSingleByObjectType(hit, start, end, objectParams, params);

	DrawDebugLine(GetWorld(), start, end, FColor::Red, false, 1.0f, 0, 1.0f);

	// If the camera raycast did not hit the object, we are too high to wall run
	return hit.GetActor() == WallRunObject;
}

void UWallRunAbility::Tick(float DeltaTime)
{
	// end conditions for wall run
	//	no need to check time since there's a timer running for that
	//	if we are on the ground, end the wall run
	//	if a raycast doesn't find the wall, end the wall run
	if (Movement->MovementMode == EMovementMode::MOVE_Walking || !WallRunContinuationRayCast())
	{
		End();
	}
	else // if we're still wall running
	{
		// increase gravity linearly based on time elapsed
		Movement->GravityScale += (DeltaTime / (WallRunMaxDuration - WallRunGravityDelay)) * (CharacterBaseGravityScale - WallRunInitialGravityScale);

		// if gravity is over CharacterBaseGravityScale, set it to CharacterBaseGravityScale and end the wall run
		if (Movement->GravityScale > CharacterBaseGravityScale)
		{
			Movement->GravityScale = CharacterBaseGravityScale;
			End();
		}
	}

	Super::Tick(DeltaTime);
}
void UWallRunAbility::Begin() 
{

}
void UWallRunAbility::End()
{
	// This can be called before the timer goes off, so check if we're actually wall running
	if (!IsActive && Movement->GravityScale == CharacterBaseGravityScale)
		return;

	/*if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Green, FString::Printf(TEXT("Start of EndWallRun ForwardVector: %s"), *GetActorForwardVector().ToString()));
	}*/

	// reset air control to default
	// 
	// TODO: We need a generic on movement ability ended custom event
	//Movement->AirControl = Movement->BaseAirControl;
	//IsWallRunning = false;
	// re-enable player rotation from input
	//bUseControllerRotationYaw = true;

	Movement->GravityScale = CharacterBaseGravityScale;
	WallRunObject = NULL;

	// TODO: use movement ability ended custom event to do this
	// set max pan angle to 60 degrees
	/*
	if (APlayerCameraManager* cameraManager = UGameplayStatics::GetPlayerCameraManager(GetWorld(), 0))
	{
		float currentYaw = Controller->GetControlRotation().Yaw;
		cameraManager->ViewYawMax = 359.998993f;
		cameraManager->ViewYawMin = 0.f;
	}
	*/

	/*if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Green, FString::Printf(TEXT("End of EndWallRun ForwardVector: %s"), *GetActorForwardVector().ToString()));
	}*/
}
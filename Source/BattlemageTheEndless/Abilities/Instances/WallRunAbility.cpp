// Fill out your copyright notice in the Description page of Project Settings.


#include "WallRunAbility.h"
#include <BattlemageTheEndless/Helpers/VectorMath.h>

FHitResult UWallRunAbility::LineTraceMovementVector(AActor* actor, USkeletalMeshComponent* mesh, FName socketName, float magnitude, bool drawTrace, FColor drawColor, float rotateYawByDegrees)
{
	FVector start = mesh->GetSocketLocation(socketName);
	// Cast a ray out in look direction magnitude units long
	FVector castVector = (FVector::XAxisVector * magnitude).RotateAngleAxis(Movement->GetLastUpdateRotation().Yaw + rotateYawByDegrees, FVector::ZAxisVector);
	FVector end = start + castVector;

	// Perform the raycast
	FHitResult hit = LineTraceGeneric(actor, start, end);

	if (drawTrace)
		DrawDebugLine(GetWorld(), start, end, drawColor, false, 3.0f, 0, 1.0f);
	return hit;
}

FHitResult UWallRunAbility::LineTraceGeneric(AActor* actor, FVector start, FVector end)
{
	// Perform the raycast
	FHitResult hit;
	FCollisionQueryParams params;
	FCollisionObjectQueryParams objectParams;
	params.AddIgnoredActor(actor);
	GetWorld()->LineTraceSingleByObjectType(hit, start, end, objectParams, params);
	return hit;
}

bool UWallRunAbility::ShouldBegin()
{
	// check if there are any eligible wallrun objects
	TArray<AActor*> overlappingActors;
	Character->GetOverlappingActors(overlappingActors, nullptr);
	for (AActor* actor : overlappingActors)
	{
		if (ObjectIsWallRunnable(actor, Mesh))
		{
			WallRunObject = actor;
			// WallRunHit is set in ObjectIsWallRunnable

			// This is used by the anim graph to determine which way to mirror the wallrun animation
			FVector start = Mesh->GetSocketLocation(FName("feetRaycastSocket"));
			// add 30 degrees to the left of the forward vector to account for wall runs starting near the start of the wall
			FVector end = start + FVector::LeftVector.RotateAngleAxis(Character->GetRootComponent()->GetComponentRotation().Yaw + 30.f, FVector::ZAxisVector) * 200;
			WallIsToLeft = LineTraceGeneric(Character, start, end).GetActor() == WallRunObject;

			return true;
		}
	}

	return false;
}

bool UWallRunAbility::ShouldEnd()
{
	// Raycast from vaultRaycastSocket straight forward to see if Object is in the way
	// TODO: Figure out if mesh can be found from actor
	FVector start = Mesh->GetSocketLocation(FName("feetRaycastSocket"));
	// Cast a ray out in hit normal direction 100 units long
	FVector castVector = (FVector::XAxisVector * 100).RotateAngleAxis(WallRunHit.ImpactNormal.RotateAngleAxis(180.f, FVector::ZAxisVector).Rotation().Yaw, FVector::ZAxisVector);
	FVector end = start + castVector;

	// Perform the raycast
	FHitResult hit;
	FCollisionQueryParams params;
	FCollisionObjectQueryParams objectParams;
	params.AddIgnoredActor(Character);
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
	if (Movement->MovementMode == EMovementMode::MOVE_Walking || !ShouldEnd())
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

	UMovementAbility::Tick(DeltaTime);
}
void UWallRunAbility::Begin()
{
	// the super should always be called first as it sets isactive
	UMovementAbility::Begin();

	CharacterBaseGravityScale = Movement->GravityScale;
	Movement->GravityScale = WallRunInitialGravityScale;

	// get vectors parallel to the wall
	FRotator possibleWallRunDirectionOne = WallRunHit.ImpactNormal.RotateAngleAxis(90.f, FVector::ZAxisVector).Rotation();
	FRotator possibleWallRunDirectionTwo = WallRunHit.ImpactNormal.RotateAngleAxis(-90.f, FVector::ZAxisVector).Rotation();

	float dirOne360 = VectorMath::NormalizeRotator0To360(possibleWallRunDirectionOne).Yaw;
	float dirTwo360 = VectorMath::NormalizeRotator0To360(possibleWallRunDirectionTwo).Yaw;
	float dirOne180 = VectorMath::NormalizeRotator180s(possibleWallRunDirectionOne).Yaw;
	float dirTwo180 = VectorMath::NormalizeRotator180s(possibleWallRunDirectionTwo).Yaw;

	float lookDirection = Movement->GetLastUpdateRotation().Yaw;

	// find the least yaw difference
	float closestDir = dirOne360;
	if (FMath::Abs(dirTwo360 - lookDirection) < FMath::Abs(closestDir - lookDirection))
	{
		closestDir = dirTwo360;
	}
	if (FMath::Abs(dirOne180 - lookDirection) < FMath::Abs(closestDir - lookDirection))
	{
		closestDir = dirOne180;
	}
	if (FMath::Abs(dirTwo180 - lookDirection) < FMath::Abs(closestDir - lookDirection))
	{
		closestDir = dirTwo180;
	}
	// consumed by caller in character
	TargetRotation = FRotator(0.f, closestDir, 0.f);

	// redirect character's velocity to be parallel to the wall, ignore input
	Movement->Velocity = TargetRotation.Vector() * Movement->Velocity.Size();
	// kill any vertical movement
	Movement->Velocity.Z = 0.f;

	// set max pan angle to 60 degrees
	if (APlayerCameraManager* cameraManager = UGameplayStatics::GetPlayerCameraManager(GetWorld(), 0))
	{
		float currentYaw = TargetRotation.Yaw;
		cameraManager->ViewYawMax = currentYaw + 90.f;
		cameraManager->ViewYawMin = currentYaw - 90.f;
	}

	// set air control to 100% to allow for continued movement parallel to the wall
	Movement->AirControl = 1.0f;

	// set a timer to end the wall run
	Character->GetWorldTimerManager().SetTimer(WallRunTimer, this, &UWallRunAbility::End, WallRunMaxDuration, false);
}
void UWallRunAbility::End()
{
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

bool UWallRunAbility::ObjectIsWallRunnable(AActor* actor, USkeletalMeshComponent* mesh)
{
	// if the object is a pawn, was cannot wallrun on it
	if (actor->IsA(APawn::StaticClass()))
		return false;

	bool drawTrace = true;

	// Repeat the same process but use socket feetRaycastSocket
	FHitResult hit = LineTraceMovementVector(actor, mesh, FName("feetRaycastSocket"), 500, drawTrace);

	bool hitLeft = false;
	bool hitRight = false;
	// If we didn't hit the object, try again with a vector 45 degress left
	if (hit.GetActor() != actor)
	{
		hit = LineTraceMovementVector(actor, mesh, FName("feetRaycastSocket"), 500, drawTrace, FColor::Blue, -45.f);

		hitLeft = hit.GetActor() == actor;
		// if we still didn't hit, try 45 right
		if (!hitLeft)
		{
			hit = LineTraceMovementVector(actor, mesh, FName("feetRaycastSocket"), 500, drawTrace, FColor::Emerald, 45.f);
			hitRight = hit.GetActor() == actor;
		}

		// We have now tried all supported directions, if we didn't hit the object, we can't wall run
		if (!hitLeft && !hitRight)
			return false;
	}

	FVector impactDirection = hit.ImpactNormal.RotateAngleAxis(180.f, FVector::ZAxisVector);
	float yawDifference = VectorMath::Vector2DRotationDifference(Movement->Velocity, impactDirection);

	// there is no need to correct for the left or right hit cases since the impact normal will be the same

	//if (GEngine)
	//	GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Green, FString::Printf(TEXT("'%f' yaw diff"), yawDifference));

	// wall run if we're more than 20 degrees rotated from the wall but less than 
	if (yawDifference > 20.f && yawDifference < 180.f)
	{
		/*if (GEngine)
			GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Green, TEXT("Starting Wall Run"));*/
		WallRunHit = hit;
		return true;
	}
	return false;
}

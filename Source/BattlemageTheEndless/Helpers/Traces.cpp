// Fill out your copyright notice in the Description page of Project Settings.

#include "Traces.h"
#include "Engine/World.h"

FHitResult Traces::LineTraceMovementVector(ACharacter* character, UCharacterMovementComponent* movement, USkeletalMeshComponent* mesh, FName socketName, float magnitude, bool drawTrace, FColor drawColor, float rotateYawByDegrees)
{
	FVector start = mesh->GetSocketLocation(socketName);
	// Cast a ray out in look direction magnitude units long
	FVector castVector = (FVector::XAxisVector * magnitude).RotateAngleAxis(movement->GetLastUpdateRotation().Yaw + rotateYawByDegrees, FVector::ZAxisVector);
	FVector end = start + castVector;

	// Perform the raycast
	FHitResult hit = LineTraceGeneric(character, start, end);

	if (drawTrace)
		DrawDebugLine(character->GetWorld(), start, end, drawColor, false, 3.0f, 0, 1.0f);
	return hit;
}

FHitResult Traces::LineTraceGeneric(AActor* actor, FVector start, FVector end)
{
	// Perform the raycast
	FHitResult hit;
	FCollisionQueryParams params;
	FCollisionObjectQueryParams objectParams;
	params.AddIgnoredActor(actor);
	actor->GetWorld()->LineTraceSingleByObjectType(hit, start, end, objectParams, params);
	return hit;
}
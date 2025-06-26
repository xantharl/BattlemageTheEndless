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
	auto params = FCollisionQueryParams(FName(TEXT("LineTrace")), true, character);
	FHitResult hit = LineTraceGeneric(character->GetWorld(), params, start, end);

	if (drawTrace)
		DrawDebugLine(character->GetWorld(), start, end, drawColor, false, 3.0f, 0, 1.0f);
	return hit;
}

FHitResult Traces::LineTraceFromCharacter(ACharacter* character, USkeletalMeshComponent* mesh, FName socketName, FRotator rotation, float magnitude, TArray<AActor*> ignoreActors, bool drawTrace, FColor drawColor)
{
	FVector start = mesh->GetSocketLocation(socketName);
	// Cast a ray out in look direction magnitude units long

	FVector castVector = rotation.Vector() * magnitude;
	//(FVector::XAxisVector * magnitude).RotateAngleAxis(rotation.Yaw, FVector::ZAxisVector).RotateAngleAxis(rotation.Pitch, FVector::XAxisVector);
	FVector end = start + castVector;

	// Perform the raycast
	auto params = FCollisionQueryParams(FName(TEXT("LineTrace")), true, character);
	params.AddIgnoredActors(ignoreActors);
	FHitResult hit = LineTraceGeneric(character->GetWorld(), params, start, end);

	if (drawTrace)
		DrawDebugLine(character->GetWorld(), start, end, drawColor, false, 3.0f, 0, 1.0f);
	return hit;
}

FHitResult Traces::LineTraceGeneric(UWorld* world, FCollisionQueryParams params, FVector start, FVector end)
{
	// Perform the raycast
	FHitResult hit;
	FCollisionObjectQueryParams objectParams;
	world->LineTraceSingleByObjectType(hit, start, end, objectParams, params);
	return hit;
}
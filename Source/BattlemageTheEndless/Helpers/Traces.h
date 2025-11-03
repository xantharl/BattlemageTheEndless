// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "CoreMinimal.h"

/**
 * 
 */
static class BATTLEMAGETHEENDLESS_API Traces
{
public:
	static FHitResult LineTraceMovementVector(ACharacter* character, UCharacterMovementComponent* movement, USkeletalMeshComponent* mesh, FName socketName, float magnitude, bool drawTrace = false, FColor drawColor = FColor::Green, float rotateYawByDegrees = 0.f);

	static FHitResult LineTraceFromCharacter(ACharacter* character, USkeletalMeshComponent* mesh, FName socketName, FRotator rotation, float magnitude, TArray<AActor*> ignoreActors, bool drawTrace = false, FColor drawColor = FColor::Green);

	/// <summary>
	/// Performs a generic line trace from start to end and ignores sourceActor
	/// </summary>
	/// <param name="params"></param>
	/// <param name="start"></param>
	/// <param name="end"></param>
	/// <returns></returns>
	static FHitResult LineTraceGeneric(UWorld* world, FCollisionQueryParams params, FVector start, FVector end, bool drawTrace = false, FColor drawColor = FColor::Green);
};

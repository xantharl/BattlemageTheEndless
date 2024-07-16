// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include <limits>

/**
 * 
 */
static class BATTLEMAGETHEENDLESS_API VectorMath
{
public:
	// returns the angle between two 2D vectors, or float::min if the vectors are invalid
	static float Vector2DRotationDifference(FVector2D A, FVector2D B);

	// returns the angle between two 2D vectors, or float::min if the vectors are invalid
	// this version takes 3d vectors and discards the z component
	static float Vector2DRotationDifference(FVector A, FVector B);

	static void NormalizeRotator(FRotator& Vector);
};

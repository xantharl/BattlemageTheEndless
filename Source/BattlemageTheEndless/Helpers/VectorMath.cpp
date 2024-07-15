// Fill out your copyright notice in the Description page of Project Settings.


#include "VectorMath.h"

float VectorMath::Vector2DRotationDifference(FVector2D A, FVector2D B)
{
	// normalize the vectors, return empty rotator on failure
	if (!A.Normalize())
		return std::numeric_limits<float>::min();

	if (!B.Normalize())
		return std::numeric_limits<float>::min();

	// get the dot product of the two vectors, then convert from radians to degrees (using pretty impreceise pi value because this isn't nasa)
	return FMath::Acos(FVector2D::DotProduct(A, B) / (A.Length() * B.Length())) * (180.f/3.14159265358979323846f);
}

float VectorMath::Vector2DRotationDifference(FVector A, FVector B)
{
	return Vector2DRotationDifference(FVector2D(A.X, A.Y), FVector2D(B.X, B.Y));
}

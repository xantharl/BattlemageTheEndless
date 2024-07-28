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

FRotator VectorMath::NormalizeRotator0To360(FRotator Rotator)
{
	// don't use .0f, use -0.00001f to account for floating point errors
	Rotator.Pitch = FMath::Fmod(Rotator.Pitch, 360.f);
	if (Rotator.Pitch < -0.00001f)
		Rotator.Pitch += 360.f;

	Rotator.Yaw = FMath::Fmod(Rotator.Yaw, 360.f);
	if (Rotator.Yaw < -0.00001f)
		Rotator.Yaw += 360.f;

	Rotator.Roll = FMath::Fmod(Rotator.Roll, 360.f);
	if (Rotator.Roll < -0.00001f)
		Rotator.Roll += 360.f;

	return Rotator;
}

FRotator VectorMath::NormalizeRotator180s(FRotator Rotator)
{
	// don't use .0f, use -0.00001f to account for floating point errors
	Rotator.Pitch = FMath::Fmod(Rotator.Pitch + 180.f, 360.f);
	if (Rotator.Pitch < -0.00001f)
		Rotator.Pitch += 360.f;
	Rotator.Pitch -= 180.f;

	Rotator.Yaw = FMath::Fmod(Rotator.Yaw + 180.f, 360.f);
	if (Rotator.Yaw < -0.00001f)
		Rotator.Yaw += 360.f;
	Rotator.Yaw -= 180.f;

	Rotator.Roll = FMath::Fmod(Rotator.Roll + 180.f, 360.f);
	if (Rotator.Roll < -0.00001f)
		Rotator.Roll += 360.f;
	Rotator.Roll -= 180.f;

	return Rotator;
}
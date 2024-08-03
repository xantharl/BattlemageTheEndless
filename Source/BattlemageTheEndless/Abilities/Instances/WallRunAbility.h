// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "../MovementAbility.h"
#include "WallRunAbility.generated.h"

/**
 * 
 */
UCLASS()
class BATTLEMAGETHEENDLESS_API UWallRunAbility : public UMovementAbility
{
	GENERATED_BODY()

public:
	// Object currently being wallrunned
	AActor* WallRunObject;
	FHitResult WallRunHit;
	FVector WallRunAttachPoint;
	FTimerHandle WallRunTimer;
	FRotator TargetRotation;
	float CharacterBaseGravityScale;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CharacterMovement, meta = (AllowPrivateAccess = "true"))
	bool WallIsToLeft;

	// Value expressed in m/s^2
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CharacterMovement, meta = (AllowPrivateAccess = "true"))
	float WallRunInitialGravityScale = 0.0f;	

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CharacterMovement, meta = (AllowPrivateAccess = "true"))
	float WallRunSpeed = 1000.f;

	/** Time before gravity starts to apply again **/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CharacterMovement, meta = (AllowPrivateAccess = "true"))
	float WallRunGravityDelay = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CharacterMovement, meta = (AllowPrivateAccess = "true"))
	float WallRunMaxDuration = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CharacterMovement, meta = (AllowPrivateAccess = "true"))
	int WallRunJumpRefundCount = 2;

	UWallRunAbility(const FObjectInitializer& X): Super(X) {Type = MovementAbilityType::WallRun;}

	// Overrides
	virtual bool ShouldBegin() override;
	virtual bool ShouldEnd() override;
	virtual void Tick(float DeltaTime) override;
	virtual void Begin() override;

	UFUNCTION(BlueprintCallable, Category = CharacterMovement)
	virtual void End() override;

	// Custom functions for just this ability
	// TODO: Move line traces to a generic helper class
	FHitResult LineTraceMovementVector(AActor* actor, USkeletalMeshComponent* mesh, FName socketName, float magnitude, bool drawTrace = false, FColor drawColor = FColor::Green, float rotateYawByDegrees = 0.f);
	FHitResult LineTraceGeneric(AActor* actor, FVector start, FVector end);
	bool ObjectIsWallRunnable(AActor* Object, USkeletalMeshComponent* mesh);
};
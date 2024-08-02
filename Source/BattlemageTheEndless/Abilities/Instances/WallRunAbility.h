// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "../MovementAbility.h"
#include "WallRunAbility.generated.h"

/**
 * 
 */
UCLASS()
class BATTLEMAGETHEENDLESS_API UWallRunAbility : public UMovementAbility
{
	GENERATED_BODY()

	// Object currently being wallrunned
	AActor* WallRunObject;
	FHitResult WallRunHit;
	FVector WallRunAttachPoint;
	FTimerHandle WallRunTimer;

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

	// Value expressed in m/s^2
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CharacterMovement, meta = (AllowPrivateAccess = "true"))
	float CharacterBaseGravityScale = 1.75f;

	bool WallRunContinuationRayCast(AActor* sourceActor, UStaticMeshComponent* mesh);

	virtual void Tick(float DeltaTime) override;

	virtual void Begin() override;

	UFUNCTION(BlueprintCallable, Category = "Character Movement")
	virtual void End() override;
};
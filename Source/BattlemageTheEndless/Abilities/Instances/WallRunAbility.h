// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "../MovementAbility.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"
#include "../../Helpers/Traces.h"
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
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = CharacterMovement, meta = (AllowPrivateAccess = "true"))
	AActor* WallRunObject;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = CharacterMovement, meta = (AllowPrivateAccess = "true"))
	FHitResult WallRunHit;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = CharacterMovement, meta = (AllowPrivateAccess = "true"))
	FTimerHandle WallRunTimer;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = CharacterMovement, meta = (AllowPrivateAccess = "true"))
	FRotator TargetRotation;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = CharacterMovement, meta = (AllowPrivateAccess = "true"))
	float CharacterBaseGravityScale;

	/** Wallrun overlap detection capsule **/
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Collision, meta = (AllowPrivateAccess = "true"))
	UCapsuleComponent* WallRunCapsule;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CharacterMovement, meta = (AllowPrivateAccess = "true"))
	bool WallIsToLeft;

	// Value expressed in m/s^2
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CharacterMovement, meta = (AllowPrivateAccess = "true"))
	float WallRunInitialGravityScale = 0.0f;	

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CharacterMovement, meta = (AllowPrivateAccess = "true"))
	float WallRunSpeed = 1000.f;

	/** Time before gravity starts to apply again **/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CharacterMovement, meta = (AllowPrivateAccess = "true"))
	float WallRunGravityDelay = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CharacterMovement, meta = (AllowPrivateAccess = "true"))
	float WallRunMaxDuration = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CharacterMovement, meta = (AllowPrivateAccess = "true"))
	int WallRunJumpRefundCount = 2;

	UWallRunAbility(const FObjectInitializer& X);

	// Overrides
	virtual void Init(UCharacterMovementComponent* movement, ACharacter* character, USkeletalMeshComponent* mesh) override;
	virtual bool ShouldBegin() override;
	virtual bool ShouldEnd() override;
	virtual void Tick(float DeltaTime) override;
	virtual void Begin(const FMovementEventData& MovementEventData) override;

	UFUNCTION(BlueprintCallable, Category = CharacterMovement)
	virtual void End(bool bForce = false) override;
	bool ObjectIsWallRunnable(AActor* Object, USkeletalMeshComponent* mesh);

	UFUNCTION(BlueprintCallable, Category = CharacterMovement)
	virtual void OnCapsuleBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
	UFUNCTION(BlueprintCallable, Category = CharacterMovement)
	virtual void OnCapsuleEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
};
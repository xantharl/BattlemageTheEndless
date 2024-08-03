// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "../MovementAbility.h"
#include "../../Helpers/Traces.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"
#include "VaultAbility.generated.h"

/**
 * 
 */
UCLASS()
class BATTLEMAGETHEENDLESS_API UVaultAbility : public UMovementAbility
{
	GENERATED_BODY()

	UVaultAbility(const FObjectInitializer& X) : Super(X) { Type = MovementAbilityType::Vault; }

	// Object currently being vaulted
	AActor* VaultTarget;
	FHitResult VaultHit;
	FVector VaultAttachPoint;
	FTimerHandle VaultEndTimer;

public:
	// Default duration is based on the animation currently in use
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CharacterMovement, meta = (AllowPrivateAccess = "true"))
	float VaultDurationSeconds = 1.1f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CharacterMovement, meta = (AllowPrivateAccess = "true"))
	bool VaultFootPlanted = false;

	float VaultElapsedTimeBeforeFootPlanted = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CharacterMovement, meta = (AllowPrivateAccess = "true"))
	float VaultEndForwardDistance = 25.0f;

	virtual void Init(UCharacterMovementComponent* movement, ACharacter* character, USkeletalMeshComponent* mesh) override;
	virtual void Begin() override;
	virtual void End() override;
	virtual bool ShouldBegin() override;
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintCallable)
	void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

};

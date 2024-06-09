// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/CapsuleComponent.h" 	
#include "Engine/DamageEvents.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "EnhancedInputComponent.h"
#include "TP_WeaponComponent.generated.h"

class ABattlemageTheEndlessCharacter;

// Declaration of the delegate that will be called when someone picks this up
// The character picking this up is the parameter sent with the notification
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FWeaponDropped);

UCLASS(Blueprintable, BlueprintType, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class BATTLEMAGETHEENDLESS_API UTP_WeaponComponent : public USkeletalMeshComponent
{
	GENERATED_BODY()

public:
	/** Delegate to whom anyone can subscribe to receive this event */
	UPROPERTY(BlueprintAssignable, Category = "Interaction")
	FWeaponDropped WeaponDropped;

	/** Projectile class to spawn */
	UPROPERTY(EditDefaultsOnly, Category=Projectile)
	TSubclassOf<class ABattlemageTheEndlessProjectile> ProjectileClass;

	/** Sound to play each time we fire */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Gameplay)
	USoundBase* FireSound;
	
	/** AnimMontage to play each time we fire */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	UAnimMontage* FireAnimation;

	/** Gun muzzle's offset from the characters location */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Gameplay)
	FVector MuzzleOffset;

	/** Gun muzzle's offset from the characters location */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Combo)
	float ComboBreakTime = 0.25f;

	/** Light attack damage */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Damage)
	float LightAttackDamage = 10.f;

	/** Light attack damage */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Damage)
	float HeavyAttackDamage = 20.f;

	/** Light attack damage */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Damage)
	float ComboFinisherMultiple = 1.5f;

	/** MappingContext */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess = "true"))
	class UInputMappingContext* FireMappingContext;

	/** Fire Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess = "true"))
	class UInputAction* FireAction;

	/** Sets default values for this component's properties */
	UTP_WeaponComponent();

	/** Make the weapon Fire a Projectile */
	UFUNCTION(BlueprintCallable, Category="Weapon")
	void Fire();

	FTimerHandle WaitForPauseTime;
	FTimerHandle TimeSinceComboPaused;

	/** The Character holding this weapon*/
	ABattlemageTheEndlessCharacter* Character;

	virtual void SuspendAttackSequence();

	void RemoveContext();

	void AddBindings();

	void DetachWeapon();
protected:
	/** Ends gameplay for this component. */
	UFUNCTION()
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual void PauseOrContinueCombo();

	virtual void EndComboIfStillPaused();

	bool AttackRequested = false;
};

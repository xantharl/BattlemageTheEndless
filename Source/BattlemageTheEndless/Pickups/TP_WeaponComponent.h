// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include <map>
#include "Components/SkeletalMeshComponent.h"
#include "Components/CapsuleComponent.h" 	
#include "Engine/DamageEvents.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "EnhancedInputComponent.h"
#include "vector"
#include <chrono>
#include "TP_WeaponComponent.generated.h"

using namespace std::chrono;

UENUM()
enum class EquipSlot : uint8
{
	Primary UMETA(DisplayName = "Primary"),
	Secondary UMETA(DisplayName = "Secondary")
};

class ABattlemageTheEndlessCharacter;

// Declaration of the delegate that will be called when someone picks this up
// The character picking this up is the parameter sent with the notification
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FWeaponDropped);

UCLASS(Blueprintable, BlueprintType, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class BATTLEMAGETHEENDLESS_API UTP_WeaponComponent : public USkeletalMeshComponent
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	EquipSlot SlotType = EquipSlot::Primary;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Abilities, meta = (AllowPrivateAccess = "true"))
	TArray<TSubclassOf<class UGameplayAbility>> GrantedAbilities;

	/** Delegate to whom anyone can subscribe to receive this event */
	UPROPERTY(BlueprintAssignable, Category = "Interaction")
	FWeaponDropped WeaponDropped;

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
	class UInputMappingContext* WeaponMappingContext;

	/** Fire Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess = "true"))
	class UInputAction* FireAction;

	/** Sets default values for this component's properties */
	UTP_WeaponComponent();

	FTimerHandle TimeSinceComboPaused;

	void RemoveContext(ACharacter* character);

	void AddBindings(ACharacter* character, UAbilitySystemComponent* abilityComponent);

	void BindAbilityActivate(FGameplayAbilitySpecHandle abilityHandle, UAbilitySystemComponent* abilityComponent);

	/** Drop Weapon Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	FVector AttachmentOffset;

	/** Light attack damage */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Damage)
	float ShotsPerMinute = 120;

	milliseconds LastFireTime;

protected:
	/** Ends gameplay for this component. */
	UFUNCTION()
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	bool AttackRequested = false;

	UFUNCTION(BlueprintCallable, Category = "Combat")
	void OnAnimTraceHit(ACharacter* character, const FHitResult& Hit);

	int ComboAttackNumber = 0;

	std::map<ABattlemageTheEndlessCharacter*, int> LastHitCharacters = std::map<ABattlemageTheEndlessCharacter*, int>();
};

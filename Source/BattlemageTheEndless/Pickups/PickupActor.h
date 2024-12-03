// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include <Components/CapsuleComponent.h>
#include "GameFramework/Actor.h"
#include "TP_WeaponComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/SkeletalMeshSocket.h"
#include "PickupActor.generated.h"

// Declaration of the delegate that will be called when someone picks this up
// The character picking this up is the parameter sent with the notification
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPickUp, ABattlemageTheEndlessCharacter*, PickUpCharacter);

/// <summary>
/// Enum of possible spell hit types
/// </summary>
UENUM(BlueprintType)
enum class EPickupType : uint8
{
	Weapon UMETA(DisplayName = "Weapon"),
	SpellClass UMETA(DisplayName = "SpellClass")
};

UCLASS()
class BATTLEMAGETHEENDLESS_API APickupActor : public AActor
{
	GENERATED_BODY()

	/** Drop Weapon Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* DropWeaponAction;

public:	
	// Sets default values for this actor's properties
	APickupActor();

	/** Delegate to whom anyone can subscribe to receive this event */
	UPROPERTY(BlueprintAssignable, Category = "Interaction")
	FOnPickUp OnPickUp;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction")
	EPickupType PickupType = EPickupType::Weapon;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Camera, meta = (AllowPrivateAccess = "true"))
	UTP_WeaponComponent* Weapon;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Camera, meta = (AllowPrivateAccess = "true"))
	UCapsuleComponent* BaseCapsule;
};

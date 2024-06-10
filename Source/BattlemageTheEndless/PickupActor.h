// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include <Components/CapsuleComponent.h>
#include "GameFramework/Actor.h"
#include "TP_WeaponComponent.h"
#include "BattlemageTheEndlessCharacter.h"
#include "EnhancedInputSubsystems.h"
#include "PickupActor.generated.h"

// Declaration of the delegate that will be called when someone picks this up
// The character picking this up is the parameter sent with the notification
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPickUp, ABattlemageTheEndlessCharacter*, PickUpCharacter);

UCLASS()
class BATTLEMAGETHEENDLESS_API APickupActor : public AActor
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Camera, meta = (AllowPrivateAccess = "true"))
	UCapsuleComponent* BaseCapsule;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Camera, meta = (AllowPrivateAccess = "true"))
	UCapsuleComponent* WeaponCollision;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Camera, meta = (AllowPrivateAccess = "true"))
	USkeletalMeshComponent* WeaponMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Camera, meta = (AllowPrivateAccess = "true"))
	UTP_WeaponComponent* Weapon;

	/** Drop Weapon Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* DropWeaponAction;

	/** Attaches the actor to a FirstPersonCharacter */
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void AttachWeapon(ABattlemageTheEndlessCharacter* TargetCharacter);	

public:	
	// Sets default values for this actor's properties
	APickupActor();

	/** Delegate to whom anyone can subscribe to receive this event */
	UPROPERTY(BlueprintAssignable, Category = "Interaction")
	FOnPickUp OnPickUp;

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void OnDropped();

	UFUNCTION(BlueprintCallable, Category = "Combat")
	virtual void OnActorHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector normalImpulse, const FHitResult& Hit);
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	/** Code for when something overlaps this component */
	UFUNCTION()
	void OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	/** The Character holding this weapon*/
	ABattlemageTheEndlessCharacter* Character;

};

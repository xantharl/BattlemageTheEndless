// Fill out your copyright notice in the Description page of Project Settings.

#include "PickupActor.h"

// Sets default values
APickupActor::APickupActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	BaseCapsule = CreateDefaultSubobject<UCapsuleComponent>(TEXT("BaseCapsule"));
	BaseCapsule->SetNotifyRigidBodyCollision(true);
	BaseCapsule->SetCollisionProfileName("OverlapAll");
	RootComponent = BaseCapsule;

	// Set Base Capsule to Collision Preset OverlapAll
	BaseCapsule->SetSimulatePhysics(true);
	BaseCapsule->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	BaseCapsule->SetCollisionResponseToAllChannels(ECR_Overlap);
	// Set Base Capsule to BlockAll for WorldStatic
	BaseCapsule->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);

	// Set up WeaponComponent and attach it to BaseCapsule
	Weapon = CreateDefaultSubobject<UTP_WeaponComponent>(TEXT("Weapon"));
	Weapon->SetupAttachment(BaseCapsule);
}

// Called when the game starts or when spawned
void APickupActor::BeginPlay()
{
	Super::BeginPlay();
}

// Called every frame
void APickupActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}
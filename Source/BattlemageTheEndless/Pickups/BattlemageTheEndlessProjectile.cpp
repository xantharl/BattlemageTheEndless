// Copyright Epic Games, Inc. All Rights Reserved.

#include "BattlemageTheEndlessProjectile.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Components/SphereComponent.h"

ABattlemageTheEndlessProjectile::ABattlemageTheEndlessProjectile() 
{
	// Use a sphere as a simple collision representation
	CollisionComp = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComp"));
	CollisionComp->InitSphereRadius(5.0f);
	CollisionComp->BodyInstance.SetCollisionProfileName("Projectile");
	CollisionComp->OnComponentHit.AddDynamic(this, &ABattlemageTheEndlessProjectile::OnHit);		// set up a notification for when this component hits something blocking

	// Players can't walk on it
	CollisionComp->SetWalkableSlopeOverride(FWalkableSlopeOverride(WalkableSlope_Unwalkable, 0.f));
	CollisionComp->CanCharacterStepUpOn = ECB_No;

	// Set as root component
	RootComponent = CollisionComp;

	// Use a ProjectileMovementComponent to govern this projectile's movement
	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileComp"));
	ProjectileMovement->UpdatedComponent = CollisionComp;
	ProjectileMovement->InitialSpeed = 3000.f;
	ProjectileMovement->MaxSpeed = 3000.f;
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->bShouldBounce = true;
	ProjectileMovement->SetIsReplicated(true);

	// Die after 3 seconds by default
	InitialLifeSpan = 3.0f;
	PrimaryActorTick.bCanEverTick = true;
	
	bReplicates = true;
	ABattlemageTheEndlessProjectile::SetReplicateMovement(true);
}

void ABattlemageTheEndlessProjectile::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	// this shouldn't be possible since we're ignoring the owner, yet we're hitting it
	if (OtherActor == OwnerActor)
		return;

	// if (GEngine) {
	// 	auto otherName = OtherActor->GetName();
	// 	GEngine->AddOnScreenDebugMessage(-1, 1.5f, FColor::Yellow, FString::Printf(TEXT("Projectile hit %s"), *otherName));
	// }
	Destroy();

	// Only add impulse and destroy projectile if we hit a physics
	if ((OtherActor != nullptr) && (OtherActor != this) && (OtherComp != nullptr) && OtherComp->IsSimulatingPhysics())
	{
		OtherComp->AddImpulseAtLocation(GetVelocity() * 100.0f, GetActorLocation());
	}
}

void ABattlemageTheEndlessProjectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// if GrowthCurve is set, scale the projectile over time
	if (ScaleByTime)
	{
		float scale = ScaleByTime->GetFloatValue(GetWorld()->GetTimeSeconds() - CreationTime);
		SetActorScale3D(FVector(scale));
	}
}
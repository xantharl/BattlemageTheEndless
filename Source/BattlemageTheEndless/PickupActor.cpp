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
	// Register our Overlap Event

	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
	WeaponMesh->SetupAttachment(BaseCapsule);

	Weapon = CreateDefaultSubobject<UTP_WeaponComponent>(TEXT("Weapon"));

	WeaponCollision = CreateDefaultSubobject<UCapsuleComponent>(TEXT("WeaponCollision"));
	WeaponCollision->SetNotifyRigidBodyCollision(true);
	WeaponCollision->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	WeaponCollision->SetCollisionProfileName("PhysicsActor");
	WeaponCollision->SetupAttachment(WeaponMesh);

	BaseCapsule->OnComponentBeginOverlap.AddDynamic(this, &APickupActor::OnSphereBeginOverlap);
}

// Called when the game starts or when spawned
void APickupActor::BeginPlay()
{
	Super::BeginPlay();
	if (WeaponMesh) 
	{
		// Set Base Capsule to dimensions of Weapon Mesh
		FVector WeaponMeshBounds = WeaponMesh->CalcBounds(WeaponMesh->GetComponentTransform()).BoxExtent;
		BaseCapsule->SetCapsuleSize(WeaponMeshBounds.X, WeaponMeshBounds.Z);
	}
}

// Called every frame
void APickupActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void APickupActor::OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// No need to check the rest if this object is already held
	if (Character != nullptr)
		return;

	// Checking if it is a First Person Character overlapping
	if (ABattlemageTheEndlessCharacter* otherCharacter = Cast<ABattlemageTheEndlessCharacter>(OtherActor))
	{
		BaseCapsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		// Unregister from the Overlap Event so it is no longer triggered
		BaseCapsule->OnComponentBeginOverlap.RemoveAll(this);

		AttachWeapon(otherCharacter);
	}
}

void  APickupActor::OnDropped()
{
	DetachWeapon();
	RootComponent->SetRelativeLocation(Character->GetActorLocation());
	BaseCapsule->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

	// Register our Overlap Event // TODO: Make this happen on a delay so it doesn't get immediately picked up
	BaseCapsule->OnComponentBeginOverlap.AddDynamic(this, &APickupActor::OnSphereBeginOverlap);

	// Null out character so we know the weapon can be picked up again
	Character = nullptr;
}

void APickupActor::AttachWeapon(ABattlemageTheEndlessCharacter* TargetCharacter)
{
	Character = TargetCharacter;
	Weapon->Character = Character;

	// Check that the character is valid, and has no rifle yet
	if (Character == nullptr || Character->GetHasWeapon())
	{
		return;
	}

	// Attach the weapon to the First Person Character
	FAttachmentTransformRules AttachmentRules(EAttachmentRule::SnapToTarget, true);
	AttachToComponent(Character->GetMesh(), AttachmentRules, FName(TEXT("GripPoint")));

	// switch bHasRifle so the animation blueprint can switch to another animation set
	Character->SetHasWeapon(Weapon);

	// Set up action bindings
	if (APlayerController* PlayerController = Cast<APlayerController>(Character->GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			// Set the priority of the mapping to 1, so that it overrides the Jump action with the Fire action when using touch input
			Subsystem->AddMappingContext(Weapon->FireMappingContext, 1);
		}

		if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerController->InputComponent))
		{
			EnhancedInputComponent->BindAction(DropWeaponAction, ETriggerEvent::Completed, this, &APickupActor::OnDropped);
		}

		Weapon->AddBindings();
	}
}


void APickupActor::DetachWeapon()
{
	// Invalid state, we should only be dropping attached weapons
	if (Character == nullptr || !Character->GetHasWeapon())
	{
		return;
	}

	DetachFromActor(FDetachmentTransformRules(EDetachmentRule::KeepRelative, false));

	Character->SetHasWeapon(nullptr);

	Weapon->RemoveContext();
	if (APlayerController* PlayerController = Cast<APlayerController>(Character->GetController()))
	{
		if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerController->InputComponent))
		{
			int removed = 0;

			int bindingCount = EnhancedInputComponent->GetNumActionBindings();
			for (int i = bindingCount - 1; i >= 0; --i)
			{
				if (Weapon->FireAction->GetName() == (EnhancedInputComponent->GetActionBinding(i)).GetActionName())
				{
					EnhancedInputComponent->RemoveActionEventBinding(i);
					removed += 1;
					// If we've removed both the Triggered and Complete bindings, we're done
					if (removed == 2)
						break;
				}
			}
		}
	}
}
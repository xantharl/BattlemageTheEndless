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
	BaseCapsule->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	BaseCapsule->SetCollisionResponseToAllChannels(ECR_Overlap);
	// Set Base Capsule to BlockAll for WorldStatic
	BaseCapsule->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);

	// Set up WeaponComponent and attach it to BaseCapsule
	Weapon = CreateDefaultSubobject<UTP_WeaponComponent>(TEXT("Weapon"));
	Weapon->SetupAttachment(BaseCapsule);

	BaseCapsule->OnComponentBeginOverlap.AddDynamic(this, &APickupActor::OnSphereBeginOverlap);
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
	// It's possible for this to be called when the weapon is not attached to a character
	//	due to a race condition in removal of action bindings
	if(!Character || !(Character->GetHasRightHandWeapon() || Character->GetHasLeftHandWeapon()))
	{
		return;
	}
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

	// Check that the character is valid, and has room for a weapon
	if (Character == nullptr || (Character->GetHasRightHandWeapon() && Character->GetHasLeftHandWeapon()))
	{
		return;
	}

	// Attach the weapon to the First Person Character
	FAttachmentTransformRules AttachmentRules(EAttachmentRule::SnapToTarget, true);
	FName targetSocketName = Character->GetTargetSocketName(Weapon->SlotType);
	bool wasWeaponSet = Character->TrySetWeapon(Weapon, targetSocketName);

	if (!wasWeaponSet) 
	{
		// Weapon slot was already occupied
		// TODO: Disable auto pickup and make it a prompt
		return;
	}

	Weapon->AttachToComponent(Character->GetMesh(), AttachmentRules, targetSocketName);

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
	Weapon->DetachFromComponent(FDetachmentTransformRules::KeepRelativeTransform);
		//DetachFromActor(FDetachmentTransformRules(EDetachmentRule::KeepRelative, false));

	Weapon->RemoveContext();
	if(Character->GetHasLeftHandWeapon())
		Character->SetLeftHandWeapon(nullptr);
	else
		Character->SetRightHandWeapon(nullptr);

	// TODO: This isn't working, figure out why
	if (APlayerController* PlayerController = Cast<APlayerController>(Character->GetController()))
	{
		if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerController->InputComponent))
		{
			int removed = 0;

			int bindingCount = EnhancedInputComponent->GetNumActionBindings();
			for (int i = bindingCount - 1; i >= 0; --i)
			{
				FName actionName = (EnhancedInputComponent->GetActionBinding(i)).GetActionName();
				if (Weapon->FireAction->GetName() == actionName || DropWeaponAction->GetName() == actionName)
				{
					EnhancedInputComponent->RemoveActionEventBinding(i);
					removed += 1;
					// If we've removed both the Triggered, Complete and Drop bindings, we're done
					// TODO: This is very fragile
					if (removed == 3)
						break;
				}
			}
		}
	}
}

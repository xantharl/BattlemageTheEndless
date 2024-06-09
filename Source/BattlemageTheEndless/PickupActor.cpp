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
	BaseCapsule->OnComponentBeginOverlap.AddDynamic(this, &APickupActor::OnSphereBeginOverlap);

	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
	WeaponMesh->SetupAttachment(BaseCapsule);

	Weapon = CreateDefaultSubobject<UTP_WeaponComponent>(TEXT("Weapon"));

	WeaponCollision = CreateDefaultSubobject<UCapsuleComponent>(TEXT("WeaponCollision"));
	WeaponCollision->SetNotifyRigidBodyCollision(true);
	WeaponCollision->SetCollisionProfileName("PhysicsActor");
	WeaponCollision->SetupAttachment(WeaponMesh);
	WeaponCollision->OnComponentHit.AddDynamic(this, &APickupActor::OnHit);

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

void APickupActor::OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	// Determine Damage`
	float Damage = Weapon->LightAttackDamage;
	// If the other actor is a game character, apply damage
	if (ABattlemageTheEndlessCharacter* otherCharacter = Cast<ABattlemageTheEndlessCharacter>(OtherActor))
	{
		otherCharacter->ApplyDamage(Damage);
	}
}

void APickupActor::OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// No need to check the rest if this object is already held
	if (Character != nullptr)
		return;

	// Checking if it is a First Person Character overlapping
	if (ABattlemageTheEndlessCharacter* otherCharacter = Cast<ABattlemageTheEndlessCharacter>(OtherActor))
	{
		//Character = otherCharacter; redundant
		// Unregister from the Overlap Event so it is no longer triggered
		WeaponCollision->OnComponentBeginOverlap.RemoveAll(this);

		AttachWeapon(otherCharacter);
	}
}

void  APickupActor::OnDropped()
{
	// Register our Overlap Event
	WeaponCollision->OnComponentBeginOverlap.AddDynamic(this, &APickupActor::OnSphereBeginOverlap);
	Weapon->DetachWeapon();
	RootComponent->SetRelativeLocation(Character->GetActorLocation());
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
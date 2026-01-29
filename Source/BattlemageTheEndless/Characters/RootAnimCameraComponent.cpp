// Fill out your copyright notice in the Description page of Project Settings.


#include "RootAnimCameraComponent.h"

#include "BattlemageTheEndlessCharacter.h"

URootAnimCameraComponent::URootAnimCameraComponent()
{
	// Ensure the actor ticks and run it in PostPhysics so Tick / TickActor is called after physics.
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.TickGroup = TG_PostPhysics;
}

void URootAnimCameraComponent::Activate(bool bReset)
{
	Owner = Cast<ACharacter>(this->GetOwner());
	if (!Owner)
	{
		UE_LOG(LogTemp, Error, TEXT("URootAnimCameraComponent::Activate failed to find ABattlemageTheEndlessCharacter Owner for component '%s'"), *GetNameSafe(this));
		return;
	}
	
	Movement = Cast<UBMageCharacterMovementComponent>(Owner->GetCharacterMovement());
	if (!Movement)
	{
		UE_LOG(LogTemp, Error, TEXT("URootAnimCameraComponent::Activate failed to find UBMageCharacterMovementComponent on Owner '%s'"), *GetNameSafe(Owner));
		return;
	}
	
	CameraComponent = Cast<USceneComponent>(Owner->FindComponentByTag(UCameraComponent::StaticClass(), FName("FirstPersonCamera")));
	if (!CameraComponent)
	{
		UE_LOG(LogTemp, Error, TEXT("URootAnimCameraComponent::Activate failed to find FirstPersonCamera on Owner '%s'"), *GetNameSafe(Owner));
		return;
	}
	
	CameraComponent->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
	
	Super::Activate(bReset);
}

void URootAnimCameraComponent::Deactivate()
{
	CameraComponent->AttachToComponent(Owner->GetMesh(), FAttachmentTransformRules::KeepRelativeTransform, FName("Head"));
	
	Owner = nullptr;
	Movement = nullptr;	
	CameraComponent = nullptr;
	
	Super::Deactivate();
}

void URootAnimCameraComponent::BeginPlay()
{
	Super::BeginPlay();
	// ...	
}

void URootAnimCameraComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);	
	
	if (!Owner || !Movement || !CameraComponent)
	{
		UE_LOG(LogTemp, Error, TEXT("URootAnimCameraComponent::TickComponent missing required references on component '%s'"), *GetNameSafe(this));
		return;
	}
	
	// Only update camera position/rotation if root motion is active or the camera is detached from the mesh
	if (!CameraComponent->IsAttachedTo(Owner->GetMesh()))
	{
		// Formula: Offset from Head attachment = FirstPersonCamera_BeginPlayPosition + HeadRadius * Look Direction Unit Vector
		float HeadRadius = 20.f; // approximate radius of head model
		auto HeadLocation = Owner->GetMesh()->GetBoneLocation(FName("Head"), EBoneSpaces::Type::WorldSpace);
		auto TargetLocation = HeadLocation + (Movement->GetForwardVector() * HeadRadius);
		CameraComponent->SetWorldLocation(TargetLocation);
		CameraComponent->SetWorldRotation(Movement->GetLastUpdateRotation());
	}
}


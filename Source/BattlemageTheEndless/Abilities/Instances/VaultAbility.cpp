// Fill out your copyright notice in the Description page of Project Settings.


#include "VaultAbility.h"

void UVaultAbility::Init(UCharacterMovementComponent* movement, ACharacter* character, USkeletalMeshComponent* mesh)
{
	UMovementAbility::Init(movement, character, mesh);
	Character->GetCapsuleComponent()->OnComponentHit.AddDynamic(this, &UVaultAbility::OnHit);
}

void UVaultAbility::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	VaultTarget = OtherActor;

	// We aren't checking for wallrun state since a vault should be able to override a wallrun
	if (!ShouldBegin())
		return;

	Begin();
}

void UVaultAbility::Begin()
{
	UMovementAbility::Begin();

	Character->DisableInput(Cast<APlayerController>(Character->Controller));
	// stop movement and gravity by enabling flying
	UCharacterMovementComponent* movement = Movement;
	movement->StopMovementImmediately();
	movement->SetMovementMode(MOVE_Flying);

	FRotator targetDirection = VaultHit.ImpactNormal.RotateAngleAxis(180.f, FVector::ZAxisVector).Rotation();

	// Rotate the character to directly face the vaulted object
	Character->Controller->SetControlRotation(targetDirection);

	// determine the highest point on the vaulted face
	// NOTE: This logic will only work for objects with flat tops
	VaultAttachPoint = VaultHit.ImpactPoint;
	// Box extent seems to be distance from origin (so we're not halving it)
	VaultAttachPoint.Z = VaultHit.Component->Bounds.Origin.Z + VaultHit.Component->Bounds.BoxExtent.Z;

	//DrawDebugSphere(GetWorld(), VaultAttachPoint, 10.0f, 12, FColor::Green, false, 1.0f, 0, 1.0f);

	// check which hand is higher
	FVector socketLocationLeftHand = Mesh->GetSocketLocation(FName("GripLeft"));
	FVector socketLocationRightHand = Mesh->GetSocketLocation(FName("GripRight"));
	FVector handToUse = socketLocationLeftHand.Z > socketLocationRightHand.Z ? socketLocationLeftHand : socketLocationRightHand;

	Character->GetRootComponent()->AddRelativeLocation(FVector(0.f, 0.f, VaultAttachPoint.Z - handToUse.Z));

	// Disable collision to allow proper movement
	Character->GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Ignore);
	Mesh->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Ignore);

	// anim graph handles the actual movement with a root motion animation to put the character in the right place
	// register a timer to end the vaulting state
	Character->GetWorldTimerManager().SetTimer(VaultEndTimer, this, &UVaultAbility::End, VaultDurationSeconds, false);
}

void UVaultAbility::End()
{
	Character->EnableInput(Cast<APlayerController>(Character->Controller));
	Movement->SetMovementMode(MOVE_Walking);
	VaultTarget = NULL;
	// anim graph handles transition back to normal state
	VaultFootPlanted = false;
	VaultElapsedTimeBeforeFootPlanted = 0;
	// turn collision with worldstatic back on
	Character->GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	Mesh->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);

	UMovementAbility::End();
}

bool UVaultAbility::ShouldBegin()
{
	// if the object is a pawn, was cannot vault it
	if (VaultTarget->IsA(APawn::StaticClass()) || Movement->MovementMode != EMovementMode::MOVE_Falling)
		return false;

	bool drawTrace = false;

	// Raycast from cameraSocket straight forward to see if Object is in the way	
	VaultHit = Traces::LineTraceMovementVector(Character, Movement, Mesh, FName("cameraSocket"), 50, drawTrace, FColor::Green, 0.f);

	// If the camera raycast hit the object, we are too low to vault
	if (VaultHit.GetActor() == VaultTarget)
		return false;

	// Repeat the same process but use socket vaultRaycastSocket
	VaultHit = Traces::LineTraceMovementVector(Character, Movement, Mesh, FName("vaultRaycastSocket"), 50, drawTrace, FColor::Green, 0.f);

	// If the vault raycast hit the object, we can vault
	return VaultHit.GetActor() == VaultTarget;
}

void UVaultAbility::Tick(float DeltaTime)
{
	FVector socketLocationToUse;
	FVector relativeLocationToAdd = FVector::Zero();
	USkeletalMeshComponent* mesh = Mesh;

	// If a foot is planted, check which one is higher
	if (VaultFootPlanted)
	{
		// check whether a foot is on top of the vaulted object
		FVector socketLocationLeftFoot = mesh->GetSocketLocation(FName("foot_l_Socket"));
		FVector socketLocationRightFoot = mesh->GetSocketLocation(FName("foot_r_Socket"));

		if (VaultAttachPoint.Z < socketLocationLeftFoot.Z)
		{
			socketLocationToUse = socketLocationLeftFoot;
		}
		else
		{
			socketLocationToUse = socketLocationRightFoot;
		}

		// if the foot is planted, move the character forward
		float timeRemaining = VaultDurationSeconds - VaultElapsedTimeBeforeFootPlanted;
		FVector forwardMotionToAdd = FVector(VaultEndForwardDistance * (DeltaTime / timeRemaining), 0.f, 0.f);
		relativeLocationToAdd += forwardMotionToAdd.RotateAngleAxis(Character->GetActorRotation().Yaw, FVector::ZAxisVector);
	}
	// otherwise, use the hand that is higher
	else
	{
		VaultElapsedTimeBeforeFootPlanted += DeltaTime;

		FVector socketLocationLeftHand = mesh->GetSocketLocation(FName("GripLeft"));
		FVector socketLocationRightHand = mesh->GetSocketLocation(FName("GripRight"));

		if (VaultAttachPoint.Z < socketLocationLeftHand.Z)
		{
			socketLocationToUse = socketLocationLeftHand;
		}
		else
		{
			socketLocationToUse = socketLocationRightHand;
		}
	}

	// adjust the character's location based on difference between socket location and attach point
	relativeLocationToAdd.Z = FMath::Max(0.f, (VaultAttachPoint - socketLocationToUse).Z);
	Character->GetRootComponent()->AddRelativeLocation(relativeLocationToAdd);
}

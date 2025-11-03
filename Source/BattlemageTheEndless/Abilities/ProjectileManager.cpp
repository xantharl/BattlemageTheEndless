// Fill out your copyright notice in the Description page of Project Settings.


#include "ProjectileManager.h"

void UProjectileManager::Initialize(ACharacter* character)
{
	OwnerCharacter = character;
	if (!OwnerCharacter)
	{
		UE_LOG(LogTemp, Error, TEXT("OwnerCharacter not set for projectile manager, cannot get lookat transform"));
		return;
	}

	if (auto controller = OwnerCharacter->GetController())
	{
		if (_ownerCameras.Num() != 0)
			_ownerCameras.Empty();

		// Look for the first active camera component and use that for the spawn transform
		for (const UActorComponent* Component : OwnerCharacter->GetComponents())
		{
			auto cameraComponent = Cast<UCameraComponent>(Component);
			if (cameraComponent)
				_ownerCameras.Add(cameraComponent);
		}
	}
}

TArray<ABattlemageTheEndlessProjectile*> UProjectileManager::SpawnProjectiles_Actor(
	UAttackBaseGameplayAbility* spawningAbility, const FProjectileConfiguration& configuration, AActor* actor = nullptr)
{
	// Default to owner if no actor was provided
	if (!actor)
	{
		actor = OwnerCharacter;
	}

	FTransform rootTransform = GetOwnerLookAtTransform();
	auto spawnLocations = GetSpawnLocations(configuration, rootTransform, spawningAbility);
	TArray<ABattlemageTheEndlessProjectile*> projectiles = HandleSpawn(spawnLocations, configuration, spawningAbility, actor);
	return projectiles;
}

TArray<ABattlemageTheEndlessProjectile*> UProjectileManager::SpawnProjectiles_Location(
	UAttackBaseGameplayAbility* spawningAbility, const FProjectileConfiguration& configuration, const FRotator rotation,
	const FVector translation, const FVector scale, AActor* ignoreActor)
{
	auto spawnLocations = GetSpawnLocations(configuration, FTransform(rotation, translation, scale), spawningAbility);
	TArray<ABattlemageTheEndlessProjectile*> projectiles = HandleSpawn(spawnLocations, configuration, spawningAbility, ignoreActor);
	return projectiles;
}

TArray<ABattlemageTheEndlessProjectile*> UProjectileManager::HandleSpawn(FTransformArrayA2& spawnLocations, const FProjectileConfiguration& configuration, 
	UAttackBaseGameplayAbility* spawningAbility, AActor* ignoreActor)
{
	if (!OwnerCharacter)
	{
		UE_LOG(LogTemp, Error, TEXT("OwnerCharacter not set for projectile manager, projectiles will not spawn"));
		return TArray<ABattlemageTheEndlessProjectile*>();
	
	}
	TArray<ABattlemageTheEndlessProjectile*> returnArray = TArray<ABattlemageTheEndlessProjectile*>();
	auto world = OwnerCharacter->GetWorld();

	if (!world)
	{
		UE_LOG(LogTemp, Error, TEXT("No world found for projectile spawning"));
		return returnArray;
	}

	FActorSpawnParameters ActorSpawnParams = FActorSpawnParameters();
	ActorSpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	TArray<AActor*> attachedActors;
	attachedActors.Add(OwnerCharacter);
	OwnerCharacter->GetAttachedActors(attachedActors, false, true);

	for (const FTransform& location : spawnLocations)
	{
		const FVector SpawnLocation = location.GetLocation();
		const FRotator SpawnRotation = location.GetRotation().Rotator();

		auto newActor = world->SpawnActor<ABattlemageTheEndlessProjectile>(
			configuration.ProjectileClass, SpawnLocation, SpawnRotation, ActorSpawnParams);

		// This sets the damage manually for a charge spell
		if (spawningAbility->ChargeDuration > 0.0f)
		{
			newActor->EffectiveDamage = spawningAbility->CurrentChargeDamage;
			newActor->ProjectileMovement->InitialSpeed *= spawningAbility->CurrentChargeProjectileSpeed;
			newActor->ProjectileMovement->ProjectileGravityScale *= spawningAbility->CurrentChargeGravityScale;
			if (GEngine)
				GEngine->AddOnScreenDebugMessage(-1, 1.5f, FColor::Yellow,
					FString::Printf(TEXT("(%s) Spawned with Damage: %f, Speed: %f, GravityScale: %f"), 
						*spawningAbility->GetName(), spawningAbility->CurrentChargeDamage, 
						newActor->ProjectileMovement->InitialSpeed, newActor->ProjectileMovement->ProjectileGravityScale));
		}

		newActor->SpawningAbility = spawningAbility;
		newActor->OwnerActor = OwnerCharacter;

		// get all actors attached to the OwnerCharacter and ignore them
		if (configuration.Shape != FSpawnShape::InwardRing)
		{
			for (AActor* actor : attachedActors)
			{
				newActor->GetCollisionComp()->IgnoreActorWhenMoving(actor, true);
			}
		}

		returnArray.Add(newActor);
	}

	// if we only have 1 projectile, we're done
	if (returnArray.Num() <= 1)
		return returnArray;

	// TODO: Figure out how to do this better than N^2 timing
	// If we have multiple, make them ignore eachother
	for (ABattlemageTheEndlessProjectile* base : returnArray)
	{
		for (ABattlemageTheEndlessProjectile* target : returnArray)
		{
			if (target == base)
				continue;

			base->GetCollisionComp()->IgnoreActorWhenMoving(target, true);
		}
	}

	return returnArray;
}

TArray<FTransform> UProjectileManager::GetSpawnLocations(const FProjectileConfiguration& configuration, const FTransform& rootTransform, UAttackBaseGameplayAbility* spawningAbility)
{
	// Some day we'll make this smarter but for now it's basically a bunch of if statements checking on the shape and location
	auto rootPlusOffset = rootTransform.GetTranslation() + configuration.SpawnOffset.RotateAngleAxis(rootTransform.Rotator().Yaw, FVector::ZAxisVector);
	auto returnArray = TArray<FTransform>();

	// set the initial lookat point naively to be max range in look direction
	auto actorLookAtTransform = GetOwnerLookAtTransform();
	auto lookAtPoint = actorLookAtTransform.GetLocation() + actorLookAtTransform.GetRotation().GetForwardVector() * spawningAbility->MaxRange;

	// If we have a character, update the lookat point to be aware of obstacles
	if (OwnerCharacter)
	{
		auto params = FCollisionQueryParams(FName(TEXT("LineTrace")), true, OwnerCharacter);
		auto hitResult = Traces::LineTraceGeneric(OwnerCharacter->GetWorld(), params, actorLookAtTransform.GetLocation(), lookAtPoint);
		if (hitResult.ImpactPoint != FVector::ZeroVector)
		{
			lookAtPoint = hitResult.ImpactPoint;
		}
	}

	FRotator lookAtRotation = UKismetMathLibrary::FindLookAtRotation(rootPlusOffset, lookAtPoint);
	auto spawnTransform = FTransform(lookAtRotation, rootPlusOffset, rootTransform.GetScale3D());

	// For shape None we will use an NGon, but that's not implemented yet
	if(configuration.Shape == FSpawnShape::None)
	{		
		returnArray.Add(spawnTransform);
	}

	// For our purposes a cone starts as a fixed sized octagon where each projectile angles out at Spread degrees.
	//    We also add interpolated projectiles between the outside and center
	else if (configuration.Shape == FSpawnShape::Cone)
	{
		// first create a projectile at the center
		returnArray.Add(spawnTransform);

		const FVector relativeLocation = FVector(0.f, ConeStartSize/2.f, 0.f);

		// use the Spread to determine the rotation of the projectile
		float halfSpread = configuration.Spread / 2.f;
		FRotator spawnRotation = FRotator(halfSpread, 0.f, 0.f);

		for (int pointNum = 0; pointNum < ConeOuterPoints; pointNum++)
		{
			// Using the center as the basis for the cone, we'll rotate the points around the center
			auto rotatedLocation = relativeLocation.RotateAngleAxis(pointNum * (360.f / (float)ConeOuterPoints), FVector::XAxisVector);

			// doing this mathematically is escaping me so we're doing it the lazy way
			auto quadrant = (pointNum / 2) + 1;
			auto firstPointInQuad = pointNum % 2 == 0;

			FRotator rotator = FRotator(0.f, 0.f, 0.f);
			if (quadrant == 1)
			{
				rotator.Pitch += halfSpread;
				if (!firstPointInQuad)
					rotator.Yaw += halfSpread;
			}
			else if (quadrant == 2)
			{
				rotator.Yaw += halfSpread;
				if (!firstPointInQuad)
					rotator.Pitch -= halfSpread;
			}
			else if (quadrant == 3)
			{
				rotator.Pitch -= halfSpread;
				if (!firstPointInQuad)
					rotator.Yaw -= halfSpread;
			}
			else if (quadrant == 4)
			{
				rotator.Yaw -= halfSpread;
				if (firstPointInQuad)
					rotator.Pitch += halfSpread;
			}

			auto thisSpawnTransform = FTransform(spawnTransform); 
			thisSpawnTransform.ConcatenateRotation(rotator.Quaternion());

			returnArray.Add(thisSpawnTransform);
		}
	}
	else if (configuration.Shape == FSpawnShape::Line)
	{
		GetLinePoints(configuration, returnArray, spawnTransform);
	}
	else if (configuration.Shape == FSpawnShape::Fan)
	{
		// use the line points function for the base points
		GetLinePoints(configuration, returnArray, spawnTransform);

		// add rotation to the points to fan them out
		auto const bEvenAmount = configuration.Amount % 2 == 0;
		int skipIndex = bEvenAmount ? 0 : 1;
		int steps = (configuration.Amount - skipIndex) / 2;

		for (int i = 0; i < configuration.Amount / 2; i++)
		{
			int firstElementIndex = i * 2 + skipIndex;
			int SecondElementIndex = (i * 2) + 1 + skipIndex;
			const float targetYaw = ((configuration.Spread / 2.f) / steps) * (i + 1);
			
			returnArray[firstElementIndex].ConcatenateRotation(FRotator(0.f, targetYaw, 0.f).Quaternion());
			returnArray[SecondElementIndex].ConcatenateRotation(FRotator(0.f, -targetYaw, 0.f).Quaternion());
		}
	}
	else if (configuration.Shape == FSpawnShape::InwardRing)
	{
		GetRingPoints(configuration, spawnTransform, returnArray, true);
	}
	else if (configuration.Shape == FSpawnShape::OutwardRing)
	{
		GetRingPoints(configuration, spawnTransform, returnArray, false);
	}

	return returnArray;
}

void UProjectileManager::GetRingPoints(const FProjectileConfiguration& configuration, const FTransform& spawnTransform, FTransformArrayA2& returnArray, bool bInwards)
{
	FVector relativeLocation = FVector(RingDiameter / 2.f, 0.f, 0.f);
	auto stepSize = 360.f / configuration.Amount;

	float rotationToAdd = bInwards ? 180.f : 0.f;
	for (int i = 0; i < configuration.Amount; i++)
	{
		auto rotatedLocation = relativeLocation.RotateAngleAxis(i * stepSize, FVector::ZAxisVector);
		auto thisSpawnTransform = FTransform(spawnTransform);
		thisSpawnTransform.AddToTranslation(rotatedLocation);
		thisSpawnTransform.ConcatenateRotation(FRotator(0.f, i * stepSize + rotationToAdd, 0.f).Quaternion());

		returnArray.Add(thisSpawnTransform);
	}
}

void UProjectileManager::GetLinePoints(const FProjectileConfiguration& configuration, FTransformArrayA2& returnArray, FTransform& spawnTransform)
{
	auto const bEvenAmount = configuration.Amount % 2 == 0;

	// if it's odd, put one right in the middle
	if (!bEvenAmount)
		returnArray.Add(spawnTransform);

	const float centerOffset = bEvenAmount ? LineSpacing / 2.f : 0.f;

	for (int i = 0; i < configuration.Amount / 2; i++)
	{
		auto offset = FVector(0.f, LineSpacing * (i + 1) + centerOffset, 0.f);
		offset = offset.RotateAngleAxis(spawnTransform.Rotator().Yaw, FVector::ZAxisVector);

		// create a spawn for both sides of center
		auto spawnTransform1 = FTransform(spawnTransform);
		auto spawnTransform2 = FTransform(spawnTransform);

		// offset them accordingly
		spawnTransform1.AddToTranslation(offset);
		spawnTransform2.AddToTranslation(offset * -1.0f);

		// add them to the return array
		returnArray.Add(spawnTransform1);
		returnArray.Add(spawnTransform2);
	}
}

void UProjectileManager::OnProjectileDestroyed(AActor* destroyedActor)
{
	//destroyedActor->Destroy();
}

FTransform UProjectileManager::GetOwnerLookAtTransform()
{
	if (!OwnerCharacter)
	{
		UE_LOG(LogTemp, Error, TEXT("OwnerCharacter not set for projectile manager, cannot get lookat transform"));
		return FTransform::Identity;
	}
	
	// prefer the first active camera component for lookat
	for (const UCameraComponent* cameraComponent : _ownerCameras)
	{
		if (!cameraComponent)
		{
			UE_LOG(LogTemp, Error, TEXT("Camera component pointer has been invalidated, removing"));
			_ownerCameras.Remove(cameraComponent);
		}
		if (cameraComponent->IsActive())
			return cameraComponent->GetComponentTransform();
	}

	// otherwise try camera socket on the mesh
	if (auto mesh = OwnerCharacter->GetMesh())
	{
		if (mesh->DoesSocketExist(FName("cameraSocket")))
		{
			return mesh->GetSocketTransform(FName("cameraSocket"));
		}
	}

	// if all else fails, return the actor transform
	return OwnerCharacter->GetActorTransform();
}
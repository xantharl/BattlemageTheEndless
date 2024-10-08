// Fill out your copyright notice in the Description page of Project Settings.


#include "ProjectileManager.h"

TArray<ABattlemageTheEndlessProjectile*> UProjectileManager::SpawnProjectiles_Actor(
	UAttackBaseGameplayAbility* spawningAbility, const FProjectileConfiguration& configuration, AActor* actor)
{
	auto returnArray = TArray<ABattlemageTheEndlessProjectile*>();
	auto spawnLocations = GetSpawnLocations(configuration, actor->GetTransform());
	TArray<ABattlemageTheEndlessProjectile*> projectiles = HandleSpawn(spawnLocations, configuration, spawningAbility, actor);
	return projectiles;
}

TArray<ABattlemageTheEndlessProjectile*> UProjectileManager::SpawnProjectiles_Location(
	UAttackBaseGameplayAbility* spawningAbility, const FProjectileConfiguration& configuration, const FRotator rotation,
	const FVector translation, const FVector scale, AActor* ignoreActor)
{
	auto returnArray = TArray<ABattlemageTheEndlessProjectile*>();
	auto spawnLocations = GetSpawnLocations(configuration, FTransform(rotation, translation, scale));
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

		newActor->SpawningAbility = spawningAbility;
		newActor->OwnerActor = OwnerCharacter;

		// get all actors attached to the OwnerCharacter and ignore them
		for (AActor* actor : attachedActors)
		{
			newActor->GetCollisionComp()->IgnoreActorWhenMoving(actor, true);
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

TArray<FTransform> UProjectileManager::GetSpawnLocations(const FProjectileConfiguration& configuration, const FTransform& rootTransform)
{
	// Some day we'll make this smarter but for now it's basically a bunch of if statements checking on the shape and location
	auto rootPlusOffset = rootTransform.GetTranslation() + configuration.SpawnOffset.RotateAngleAxis(rootTransform.Rotator().Yaw, FVector::ZAxisVector);
	auto returnArray = TArray<FTransform>();

	// For shape None we will use an NGon, but that's not implemented yet
	if(configuration.Shape == FSpawnShape::None)
	{		
		auto spawnTransform = FTransform(rootTransform.GetRotation(), rootPlusOffset, rootTransform.GetScale3D());
		returnArray.Add(spawnTransform);
	}

	// For our purposes a cone starts as a fixed sized octagon where each projectile angles out at Spread degrees.
	//    We also add interpolated projectiles between the outside and center
	else if (configuration.Shape == FSpawnShape::Cone)
	{
		// first create a projectile at the center
		auto const spawnTransform = FTransform(rootTransform.GetRotation(), rootPlusOffset, rootTransform.GetScale3D());
		returnArray.Add(spawnTransform);

		const FVector relativeLocation = FVector(0.f, ConeStartSize/2.f, 0.f);
		const auto controlRotation = OwnerCharacter->GetControlRotation();

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

	return returnArray;
}

void UProjectileManager::OnProjectileDestroyed(AActor* destroyedActor)
{
	//destroyedActor->Destroy();
}

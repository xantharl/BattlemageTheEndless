// Fill out your copyright notice in the Description page of Project Settings.


#include "GameplayCueNotify_ActorWNiagara.h"

AGameplayCueNotify_ActorWNiagara::AGameplayCueNotify_ActorWNiagara()
{
	NiagaraConfig = CreateDefaultSubobject<UNiagaraComponent>(TEXT("NiagaraConfig"));
}

void AGameplayCueNotify_ActorWNiagara::TickActor(float DeltaTime, ELevelTick TickType, FActorTickFunction& ThisTickFunction)
{
	UWorld* const world = GetWorld();
	if (!world)
	{
		UE_LOG(LogTemp, Warning, TEXT("No world found in AGameplayCueNotify_ActorWNiagara::TickActor"));
		return;
	}

	UNiagaraSystem* NiagaraSystem = NiagaraConfig->GetAsset();
	if (NiagaraSystem && !NiagaraInstance)
	{
		// get the character and be safe about it
		auto controller = world->GetFirstPlayerController();
		if (!controller) {
			UE_LOG(LogTemp, Warning, TEXT("No controller found in AGameplayCueNotify_ActorWNiagara::TickActor"));
			return;
		}
		auto pawn = world->GetFirstPlayerController()->AcknowledgedPawn;
		if (!pawn) {
			UE_LOG(LogTemp, Warning, TEXT("No pawn found in AGameplayCueNotify_ActorWNiagara::TickActor"));
			return;
		}
		auto character = Cast<ACharacter>(world->GetFirstPlayerController()->AcknowledgedPawn);
		if (!character) {
			UE_LOG(LogTemp, Warning, TEXT("No character found in AGameplayCueNotify_ActorWNiagara::TickActor"));
			return;
		}


		const FRotator SpawnRotation = controller->PlayerCameraManager->GetCameraRotation() + NiagaraConfig->GetRelativeTransform().Rotator();
		FVector SpawnLocation = NiagaraConfig->GetRelativeLocation().RotateAngleAxis(GetTransform().Rotator().Yaw, FVector::ZAxisVector);

		// handle bSnapToGround
		if (AttackEffect.bSnapToGround)
		{
			FHitResult hitResult;
			const FVector end = SpawnLocation - FVector(0.f, 0.f, 1000.f);
			GetWorld()->LineTraceSingleByChannel(hitResult, SpawnLocation, end, ECollisionChannel::ECC_Visibility);
			if (hitResult.bBlockingHit)
			{
				SpawnLocation = hitResult.ImpactPoint;
			}
		}

		//Set Spawn Collision Handling Override
		FActorSpawnParameters ActorSpawnParams;
		ActorSpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		if (AttackEffect.AttachSocket.IsNone())
		{
			NiagaraInstance = UNiagaraFunctionLibrary::SpawnSystemAtLocation(world, NiagaraSystem,
				SpawnLocation, GetTransform().Rotator(), FVector(1.f), false, true, ENCPoolMethod::None, false);
		}
		else
		{
			NiagaraInstance = UNiagaraFunctionLibrary::SpawnSystemAttached(NiagaraSystem, character->GetMesh(),
				AttackEffect.AttachSocket, FVector::ZeroVector, FRotator::ZeroRotator, EAttachLocation::KeepRelativeOffset, false,
				true, ENCPoolMethod::None, false);
		}
	}

	Super::TickActor(DeltaTime, TickType, ThisTickFunction);
}
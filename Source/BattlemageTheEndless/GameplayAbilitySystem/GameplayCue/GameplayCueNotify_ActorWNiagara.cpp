// Fill out your copyright notice in the Description page of Project Settings.


#include "GameplayCueNotify_ActorWNiagara.h"
#include <BattlemageTheEndless/Characters/BattlemageTheEndlessCharacter.h>

AGameplayCueNotify_ActorWNiagara::AGameplayCueNotify_ActorWNiagara()
{
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	NiagaraConfig = CreateDefaultSubobject<UNiagaraComponent>(TEXT("NiagaraConfig"));
	NiagaraConfig->AttachToComponent(SceneRoot, FAttachmentTransformRules::KeepRelativeTransform);

	NiagaraInstance = nullptr;
}

void AGameplayCueNotify_ActorWNiagara::HandleGameplayCue(AActor* MyTarget, EGameplayCueEvent::Type EventType, const FGameplayCueParameters& Parameters)
{
	// if the EventType is OnActive, we want to spawn the NiagaraSystem
	if (EventType == EGameplayCueEvent::OnActive)
	{
		TryCreateNiagaraInstance();

		if (AttackEffect.bShouldKillPreviousEffect)
		{
			if (ABattlemageTheEndlessCharacter* character = Cast< ABattlemageTheEndlessCharacter>(MyTarget))
			{
				character->TryRemovePreviousAbilityEffect(this);
			}
		}
	}
	else if (EventType == EGameplayCueEvent::Removed)
		TryDestroyNiagaraInstance();

	Super::HandleGameplayCue(MyTarget, EventType, Parameters);
}

void AGameplayCueNotify_ActorWNiagara::TryDestroyNiagaraInstance()
{
	if (NiagaraInstance)
	{
		NiagaraInstance->Deactivate();
		NiagaraInstance->DestroyComponent();
		NiagaraInstance = nullptr;
	}
}

void AGameplayCueNotify_ActorWNiagara::TryCreateNiagaraInstance()
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


		FRotator SpawnRotation = controller->PlayerCameraManager->GetCameraRotation() + NiagaraConfig->GetRelativeTransform().Rotator();
		SpawnRotation.Roll = 0.f;
		//FVector SpawnLocation = NiagaraConfig->GetRelativeLocation().RotateAngleAxis(GetTransform().Rotator().Yaw, FVector::ZAxisVector);
		FVector SpawnLocation = character->GetActorLocation() 
			+ NiagaraConfig->GetRelativeTransform().GetLocation().RotateAngleAxis(controller->PlayerCameraManager->GetCameraRotation().Yaw, FVector::ZAxisVector);

		// handle bSnapToGround
		if (AttackEffect.bSnapToGround)
		{
			FHitResult hitResult;
			const FVector end = SpawnLocation - FVector(0.f, 0.f, 1000.f);
			GetWorld()->LineTraceSingleByChannel(hitResult, SpawnLocation, end, ECollisionChannel::ECC_Visibility);
			SpawnRotation.Pitch = 0.f;
			if (hitResult.bBlockingHit)
			{
				// Add a small offset to prevent the effect from being blocked by the ground
				SpawnLocation = hitResult.ImpactPoint + FVector(0.f, 0.f, 0.01f);
			}
		}

		//Set Spawn Collision Handling Override
		FActorSpawnParameters ActorSpawnParams;
		ActorSpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		if (AttackEffect.AttachSocket.IsNone())
		{
			NiagaraInstance = UNiagaraFunctionLibrary::SpawnSystemAtLocation(world, NiagaraSystem,
				SpawnLocation, SpawnRotation, NiagaraConfig->GetRelativeTransform().GetScale3D(), false, true, ENCPoolMethod::None, false);
		}
		else
		{
			NiagaraInstance = UNiagaraFunctionLibrary::SpawnSystemAttached(NiagaraSystem, character->GetMesh(),
				AttackEffect.AttachSocket, FVector::ZeroVector, SpawnRotation, EAttachLocation::KeepRelativeOffset, false,
				true, ENCPoolMethod::None, false);
		}
	}
}

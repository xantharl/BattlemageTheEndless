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
		TryCreateNiagaraInstance(MyTarget);

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

	Destroy();
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

void AGameplayCueNotify_ActorWNiagara::TryCreateNiagaraInstance(AActor* MyTarget)
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
		FRotator SpawnRotation = MyTarget->GetRootComponent()->GetRelativeRotation() + NiagaraConfig->GetRelativeTransform().Rotator();
		SpawnRotation.Roll = 0.f;
		//FVector SpawnLocation = NiagaraConfig->GetRelativeLocation().RotateAngleAxis(GetTransform().Rotator().Yaw, FVector::ZAxisVector);
		FVector SpawnLocation = MyTarget->GetActorLocation() 
			+ NiagaraConfig->GetRelativeTransform().GetLocation().RotateAngleAxis(MyTarget->GetRootComponent()->GetRelativeRotation().Yaw, FVector::ZAxisVector);

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

		if (AttackEffect.AttachType == EAttachType::None)
		{
			NiagaraInstance = UNiagaraFunctionLibrary::SpawnSystemAtLocation(world, NiagaraSystem,
				SpawnLocation, SpawnRotation, NiagaraConfig->GetRelativeTransform().GetScale3D(), false, true, ENCPoolMethod::None, false);
			return;
		}

		if (AttackEffect.AttachSocket.IsNone())
		{
			UE_LOG(LogTemp, Warning, TEXT("AttachSocket is not set for GameplayCue %s, system will not be spawned"), *GetName());
			return;
		}

		USkeletalMeshComponent* attachToComponent;
		if (AttackEffect.AttachType == EAttachType::Character)
		{
			ACharacter* character = Cast<ACharacter>(MyTarget);
			attachToComponent = character->GetMesh();
		}
		else
		{
			ABattlemageTheEndlessCharacter* character = Cast<ABattlemageTheEndlessCharacter>(MyTarget);
			attachToComponent = character->GetWeapon(
				AttackEffect.AttachType == EAttachType::PrimaryWeapon ? EquipSlot::Primary : EquipSlot::Secondary)->Weapon;
		}

		NiagaraInstance = UNiagaraFunctionLibrary::SpawnSystemAttached(NiagaraSystem, attachToComponent,
			AttackEffect.AttachSocket, FVector::ZeroVector, SpawnRotation, EAttachLocation::KeepRelativeOffset, 
			false,true, ENCPoolMethod::None, false);
		
	}
}

void AGameplayCueNotify_ActorWNiagara::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
}

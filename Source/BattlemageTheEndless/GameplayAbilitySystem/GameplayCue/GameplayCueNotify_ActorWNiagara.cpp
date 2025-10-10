// Fill out your copyright notice in the Description page of Project Settings.


#include "GameplayCueNotify_ActorWNiagara.h"
#include <BattlemageTheEndless/Characters/BattlemageTheEndlessCharacter.h>

AGameplayCueNotify_ActorWNiagara::AGameplayCueNotify_ActorWNiagara()
{
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	ParticleSystem = CreateDefaultSubobject<UNiagaraComponent>(TEXT("NiagaraConfig"));
	ParticleSystem->AttachToComponent(SceneRoot, FAttachmentTransformRules::KeepRelativeTransform);

	// disable auto activation, we want to control when the particle system is activated
	// This is because GAS creates separate instances of the gameplay cue for the immediate application
	// and the duration effect, and we only want to spawn the particle system once
	ParticleSystem->bAutoActivate = false;

	// makes the particle system follow the owner (target) of the gameplay cue
	bAutoAttachToOwner = true;

	if (AttackEffect.AttachSocket.IsNone())
	{
		UE_LOG(LogTemp, Warning, TEXT("AttachSocket is not set for GameplayCue %s, system will not be spawned"), *GetName());
		return;
	}
}

void AGameplayCueNotify_ActorWNiagara::HandleGameplayCue(AActor* MyTarget, EGameplayCueEvent::Type EventType, const FGameplayCueParameters& Parameters)
{
	// if the EventType is OnActive, we want to spawn the NiagaraSystem
	if (EventType == EGameplayCueEvent::OnActive)
	{
		ParticleSystem->Activate();

		UMeshComponent* attachToComponent;
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

		AttachToComponent(attachToComponent, FAttachmentTransformRules::KeepRelativeTransform, AttackEffect.AttachSocket);

		ABattlemageTheEndlessCharacter* character = Cast<ABattlemageTheEndlessCharacter>(MyTarget);
		if (character && AttackEffect.bShouldKillPreviousEffect)
		{
			character->TryRemovePreviousAbilityEffect(this);
		}
	}

	Super::HandleGameplayCue(MyTarget, EventType, Parameters);

	if (EventType == EGameplayCueEvent::Removed)
	{
		ParticleSystem->Deactivate();
		Super::Destroy();
	}
}

void AGameplayCueNotify_ActorWNiagara::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
}

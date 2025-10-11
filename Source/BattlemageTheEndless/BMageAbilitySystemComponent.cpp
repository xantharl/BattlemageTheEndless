// Fill out your copyright notice in the Description page of Project Settings.


#include "BMageAbilitySystemComponent.h"
using namespace std::chrono;

UBMageAbilitySystemComponent::UBMageAbilitySystemComponent()
{
	_markSphere = CreateDefaultSubobject<USphereComponent>(TEXT("MarkSphere"));
	_markSphere->OnComponentBeginOverlap.AddDynamic(this, &UBMageAbilitySystemComponent::OnMarkSphereBeginOverlap);
	_markSphere->OnComponentEndOverlap.AddDynamic(this, &UBMageAbilitySystemComponent::OnMarkSphereEndOverlap);
	_markSphere->SetCollisionResponseToAllChannels(ECR_Overlap);
}

void UBMageAbilitySystemComponent::BeginPlay()
{
	Super::BeginPlay();
	// safety check since the bp editor can call this
	if (GetWorld())
	{
		_markSphere->SetVisibility(true);
		_markSphere->SetHiddenInGame(false);
		_markSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		_markSphere->SetGenerateOverlapEvents(false);
		_markSphere->RegisterComponent();
		_markSphere->AttachToComponent(GetOwner()->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
	}

	OnAnyGameplayEffectRemovedDelegate().AddUObject(this, &UBMageAbilitySystemComponent::OnRemoveGameplayEffectCallback);
}

void UBMageAbilitySystemComponent::MarkOwner(AActor* instigator, float duration, float range, UMaterialInstance* markedMaterial, UMaterialInstance* markedMaterialOutOfRange)
{
	// Add overlay material to target
	USkeletalMeshComponent* TargetMesh = Cast<USkeletalMeshComponent>(GetOwner()->GetComponentByClass(USkeletalMeshComponent::StaticClass()));

	_instigator = instigator;
	_markedMaterial = markedMaterial;
	_markedMaterialOutOfRange = markedMaterialOutOfRange;

	auto bInRange = FVector::Dist(instigator->GetActorLocation(), GetOwner()->GetActorLocation()) <= range;

	if (TargetMesh)
	{
		TargetMesh->SetOverlayMaterial(bInRange ? _markedMaterial : _markedMaterialOutOfRange);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("ExecutionCalculation_Marked: Missing Marked Material"));
		return;
	}

	// Add temporary collision volume to target based on range
	_markSphere->SetSphereRadius(range);
	_markSphere->SetGenerateOverlapEvents(true);
	_markSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

	AddLooseGameplayTag(FGameplayTag::RequestGameplayTag("GameplayCue.Status.Marked"));

	GetWorld()->GetTimerManager().SetTimer(_unmarkTimerHandle, this, &UBMageAbilitySystemComponent::UnmarkOwner, duration, false);
}

void UBMageAbilitySystemComponent::UnmarkOwner()
{
	// Remove overlay material from owner
	USkeletalMeshComponent* TargetMesh = Cast<USkeletalMeshComponent>(GetOwner()->GetComponentByClass(USkeletalMeshComponent::StaticClass()));
	if (TargetMesh)
	{
		TargetMesh->SetOverlayMaterial( nullptr);
	}

	_markSphere->SetGenerateOverlapEvents(false);
	_markSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	RemoveLooseGameplayTag(FGameplayTag::RequestGameplayTag("GameplayCue.Status.Marked"));
}

void UBMageAbilitySystemComponent::OnMarkSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (OtherActor != _instigator)
		return;

	USkeletalMeshComponent* TargetMesh = Cast<USkeletalMeshComponent>(GetOwner()->GetComponentByClass(USkeletalMeshComponent::StaticClass()));
	if (TargetMesh)
	{
		TargetMesh->SetOverlayMaterial(_markedMaterial);
	}
}

void UBMageAbilitySystemComponent::OnMarkSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (OtherActor != _instigator)
		return;

	USkeletalMeshComponent* TargetMesh = Cast<USkeletalMeshComponent>(GetOwner()->GetComponentByClass(USkeletalMeshComponent::StaticClass()));
	if (TargetMesh)
	{
		TargetMesh->SetOverlayMaterial(_markedMaterialOutOfRange);
	}
}

void UBMageAbilitySystemComponent::OnRemoveGameplayEffectCallback(const FActiveGameplayEffect& EffectRemoved)
{
	if (GEngine)
		GEngine->AddOnScreenDebugMessage(-1, 1.5f, FColor::Yellow, FString::Printf(TEXT("Effect %s removed"), *EffectRemoved.Spec.Def->GetName()));

	// if the applying ability is still active, check if this was the last effect
		// Find all active effects on this ASC with the same 
}

TObjectPtr<UGameplayAbility> UBMageAbilitySystemComponent::GetActivatableAbilityByOwnedTag(FName abilityTag) 
{
	for (const FGameplayAbilitySpec& Spec : GetActivatableAbilities())
	{
		if (!Spec.Ability)
			continue;

		if (Spec.Ability->AbilityTags.HasTag(FGameplayTag::RequestGameplayTag(abilityTag)))
		{
			return Spec.Ability;
		}
	}

	return nullptr;
}
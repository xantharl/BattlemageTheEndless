// Fill out your copyright notice in the Description page of Project Settings.


#include "HitEffectActor.h"

// Sets default values
AHitEffectActor::AHitEffectActor()
{
	if (!Validate())
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::Red, TEXT("HitEffectActor failed to validate, see log for more info"));
		}
	}

 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	InitCollisionObject();
	InitCollisionType();
}

void AHitEffectActor::InitCollisionObject()
{
	TObjectPtr<USphereComponent> area = CreateDefaultSubobject<USphereComponent>(TEXT("AreaOfEffect"));
	area->SetSphereRadius(100.f);
	area->InitSphereRadius(100.f);
	AreaOfEffect = area;
	SetRootComponent(AreaOfEffect);
}

void AHitEffectActor::InitCollisionType()
{
	AreaOfEffect->SetCollisionProfileName(TEXT("OverlapAll"));
	AreaOfEffect->SetGenerateOverlapEvents(true);
	
	if (bRootShouldApplyEffects)
	{
		AreaOfEffect->OnComponentBeginOverlap.AddDynamic(this, &AHitEffectActor::OnAreaOfEffectBeginOverlap);
		AreaOfEffect->OnComponentEndOverlap.AddDynamic(this, &AHitEffectActor::OnAreaOfEffectEndOverlap);
	}
}

void AHitEffectActor::SnapActorToGround(FHitResult hitResult)
{
	if (!SnapToGround)
		return;
		
	// If we didn't hit anything or we hit a non primitive, do a line trace down to find the ground
	if (!hitResult.GetComponent())
	{
		FVector start = GetActorLocation();
		FVector end = start - FVector(0, 0, 1000);
		FCollisionQueryParams params;
		params.AddIgnoredActor(this);
		GetWorld()->LineTraceSingleByChannel(hitResult, start, end, ECC_Visibility, params);
	}
	// If we did hit something, use that hit result to snap the actor to the "ground", this can be any surface 
	if (hitResult.GetComponent())
	{
		auto location = hitResult.Location;
		// add height so ground effects don't clip into the ground
		location.Z += 1.f;
		SetActorLocation(location);

		// account for uneven surfaces, walls, ceilings
		auto adjustRot = hitResult.ImpactNormal.Rotation() - FVector::UpVector.Rotation();

		// preserve yaw if we are on the floor or ceiling
		if (hitResult.ImpactNormal == FVector::UpVector || hitResult.ImpactNormal == FVector::DownVector)
			adjustRot.Yaw = GetActorRotation().Yaw;

		SetActorRotation(adjustRot);
	}
}

void AHitEffectActor::BeginPlay()
{
	Super::BeginPlay();
	if (!AreaOfEffect)
		return;
	// Check for any collision components and register their overlap events
	auto components = this->GetComponents();
	for (UActorComponent* child : components)
	{
		if (child == GetRootComponent())
			continue;

		auto childPrimitive = Cast<UShapeComponent>(child);
		if (childPrimitive)
		{
			childPrimitive->SetCollisionProfileName(TEXT("OverlapAll"));
			childPrimitive->SetGenerateOverlapEvents(true);

			// safety checks since something is binding this for AreaOfEffect already
			if (!childPrimitive->OnComponentBeginOverlap.IsBound())
				childPrimitive->OnComponentBeginOverlap.AddDynamic(this, &AHitEffectActor::OnAreaOfEffectBeginOverlap);
			if (!childPrimitive->OnComponentEndOverlap.IsBound())
				childPrimitive->OnComponentEndOverlap.AddDynamic(this, &AHitEffectActor::OnAreaOfEffectEndOverlap);
		}
	}
}

void AHitEffectActor::ActivateEffect(TObjectPtr<UNiagaraSystem> system)
{
	_visualEffectInstance = UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), system,
		GetActorLocation(), GetActorRotation(), VisualEffectScale, false, true, ENCPoolMethod::None, false);
}

void AHitEffectActor::DeactivateEffect()
{
	if (_visualEffectInstance)
	{
		_visualEffectInstance->Deactivate();
		_visualEffectInstance->DestroyComponent();
		_visualEffectInstance = nullptr;
	}
}

void AHitEffectActor::Destroyed()
{
 	DeactivateEffect();
	Super::Destroyed();
}

bool AHitEffectActor::Validate()
{
	return true;

	// TODO: this function is currently just logging overload, revisit it later if we want to use it
	//if (!AreaOfEffect)
	//{
	//	UE_LOG(LogTemp, Error, TEXT("AreaOfEffect is null in HitEffectActor"));
	//	return false;
	//}
	//else if (!VisualEffectSystem)
	//{
	//	UE_LOG(LogTemp, Error, TEXT("VisualEffect is null in HitEffectActor"));
	//	return false;
	//}
	//else if (Effects.Num() == 0)
	//{
	//	UE_LOG(LogTemp, Error, TEXT("Effects array is empty in HitEffectActor"));
	//	return false;
	//}

	//return true;
}

void AHitEffectActor::ApplyEffects(AActor* actor, UPrimitiveComponent* applyingComponent)
{
	auto abilitySystemComponent = actor->FindComponentByClass<UAbilitySystemComponent>();
	if (!abilitySystemComponent)
		return;

	for (TSubclassOf<UGameplayEffect> effect : Effects)
	{
		FGameplayEffectContextHandle context = abilitySystemComponent->MakeEffectContext();
		if (Instigator)
		{
			context.AddSourceObject(Instigator);
			context.AddInstigator(Instigator, Instigator);
			context.AddActors({ Instigator });
		}

		FGameplayEffectSpecHandle specHandle = abilitySystemComponent->MakeOutgoingSpec(effect, 1.f, context);
		// check if the effect in question is already applied to the target
		//TArray<FGameplayEffectSpec> OutSpecCopies;
		//abilitySystemComponent->GetAllActiveGameplayEffectSpecs(OutSpecCopies);

		//auto existingSpec = OutSpecCopies.FindByPredicate([effect](const FGameplayEffectSpec& spec) { return spec.Def.IsA(effect); });

		//if (existingSpec)
		//{
		//	if (GEngine)
		//	{
		//		GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::Green, 
		//			FString::Printf(TEXT("Effect %s already applied to target, adding stack if possible"), *effect->GetName()));
		//	}
		//	existingSpec->SetStackCount(existingSpec->GetStackCount() + 1);
		//	abilitySystemComponent->Stack
		//	return;
		//}

		// If we haven't returned, this is a new effect so apply it

		if (!specHandle.IsValid())
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to create effect spec for %s"), *effect->GetName());
			continue;
		}

		auto handle = abilitySystemComponent->ApplyGameplayEffectSpecToSelf(*specHandle.Data.Get());
		if (GEngine)
		{
			int stacks = abilitySystemComponent->GetCurrentStackCount(handle);
			GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::Green, 
				FString::Printf(TEXT("Applied effect %s (Stacks: %i)"), *effect->GetName(), stacks));
		}
	}

	if (OnApplyEffectsBehavior == EOnApplyEffectsBehavior::DestroyActor)
	{
		Destroy();
	}
	else if (OnApplyEffectsBehavior == EOnApplyEffectsBehavior::DestroyComponent)
	{
		// if we have been passed a collision volume, get its parent and destroy that
		auto collisionComponent = Cast<UShapeComponent>(applyingComponent);
		if (collisionComponent)
		{
			auto parent = collisionComponent->GetAttachParent();
			// we have to destroy both the collision component and its parent since children do not get auto-destroyed
			collisionComponent->DestroyComponent();
			if (parent)
				parent->DestroyComponent();
		}
		else
		{
			// otherwise just destroy the component we were passed, though this probably isn't a real use case
			applyingComponent->DestroyComponent();
		}

		// if we are all out of colliding components, destroy ourselves
		auto components = this->GetComponents();
		bool hasCollision = false;
		for (UActorComponent* child : components)
		{
			auto childPrimitive = Cast<UShapeComponent>(child);
			if (childPrimitive == GetRootComponent() && !bRootShouldApplyEffects)
				continue;
			if (childPrimitive && childPrimitive->IsCollisionEnabled())
			{
				hasCollision = true;
				break;
			}
		}

		if (!hasCollision)
			Destroy();
	}
}

void AHitEffectActor::OnAreaOfEffectBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// special case for actors that act as buffs for players since we still need to anchor around the root component, but it's not what we want to apply effects from
	if (OverlappedComponent == GetRootComponent() && !bRootShouldApplyEffects)
		return;

	// TODO: We might want some spells to be able to hit the owner, but for now we're disabling that
	// technically, "Self" spellos hit the owner, but we won't have hit effect actors on those
	if (Instigator && OtherActor == Instigator)
		return;

	// if OtherActor does not have an AbilitySystemComponent, do nothing
	if (!OtherActor || OtherActor == this || !OtherActor->FindComponentByClass<UAbilitySystemComponent>())
		return;

	// if OtherComp is not the root component, do nothing
	if (OtherComp && OtherComp != OtherActor->GetRootComponent())
		return;

	// if we aren't re-applying, apply the effects now and be done
	if (EffectApplicationInterval <= 0.0001f)
	{
		ApplyEffects(OtherActor, OverlappedComponent);
		return;
	}

	// Otherwise use a reocurring timer to apply the effects
	_effectReapplyTimers.Add(OtherActor, FTimerHandle());
	auto timerDelegate = FTimerDelegate::CreateUObject(this, &AHitEffectActor::ApplyEffects, OtherActor, OverlappedComponent);
	GetWorld()->GetTimerManager().SetTimer(_effectReapplyTimers[OtherActor], timerDelegate, EffectApplicationInterval, true);
}

void AHitEffectActor::OnAreaOfEffectEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	// If we are currently tracking this actor, stop doing so
	if (_effectReapplyTimers.Contains(OtherActor))
		GetWorld()->GetTimerManager().ClearTimer(_effectReapplyTimers[OtherActor]);
}
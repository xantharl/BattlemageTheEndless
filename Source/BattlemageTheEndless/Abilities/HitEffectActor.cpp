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
	AreaOfEffect = CreateDefaultSubobject<USphereComponent>(TEXT("AreaOfEffect"));
	SetRootComponent(AreaOfEffect);
	AreaOfEffect->SetSphereRadius(100.f);
	AreaOfEffect->InitSphereRadius(100.f);
	AreaOfEffect->SetCollisionProfileName(TEXT("OverlapAll"));
	AreaOfEffect->SetGenerateOverlapEvents(true);
	AreaOfEffect->OnComponentBeginOverlap.AddDynamic(this, &AHitEffectActor::OnAreaOfEffectBeginOverlap);
	AreaOfEffect->OnComponentEndOverlap.AddDynamic(this, &AHitEffectActor::OnAreaOfEffectEndOverlap);
}

void AHitEffectActor::SnapActorToGround()
{
	if (SnapToGround)
	{
		FHitResult hitResult;
		FVector start = GetActorLocation();
		FVector end = start - FVector(0, 0, 1000);
		FCollisionQueryParams params;
		params.AddIgnoredActor(this);
		if (GetWorld()->LineTraceSingleByChannel(hitResult, start, end, ECC_Visibility, params))
		{
			auto location = hitResult.Location;
			// add height so ground effects don't clip into the ground
			location.Z += 1.f;
			SetActorLocation(location);
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
	if (!AreaOfEffect)
	{
		UE_LOG(LogTemp, Error, TEXT("AreaOfEffect is null in HitEffectActor"));
		return false;
	}
	else if (!VisualEffectSystem)
	{
		UE_LOG(LogTemp, Error, TEXT("VisualEffect is null in HitEffectActor"));
		return false;
	}
	else if (Effects.Num() == 0)
	{
		UE_LOG(LogTemp, Error, TEXT("Effects array is empty in HitEffectActor"));
		return false;
	}

	return true;
}

void AHitEffectActor::ApplyEffects(AActor* actor)
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
		if (specHandle.IsValid())
		{
			auto handle = abilitySystemComponent->ApplyGameplayEffectSpecToSelf(*specHandle.Data.Get());
			if (GEngine)
			{
				GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::Green, 
					FString::Printf(TEXT("Applied effect %s (Stacks: %i)"), *effect->GetName(), 
						abilitySystemComponent->GetCurrentStackCount(handle)));
			}
		}
	}
}

void AHitEffectActor::OnAreaOfEffectBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// if we aren't re-applying, apply the effects now and be done
	if (EffectApplicationInterval <= 0.0001f)
	{
		ApplyEffects(OtherActor);
		return;
	}

	// Otherwise use a reocurring timer to apply the effects
	_effectReapplyTimers.Add(OtherActor, FTimerHandle());
	auto timerDelegate = FTimerDelegate::CreateUObject(this, &AHitEffectActor::ApplyEffects, OtherActor);
	GetWorld()->GetTimerManager().SetTimer(_effectReapplyTimers[OtherActor], timerDelegate, EffectApplicationInterval, true);
}

void AHitEffectActor::OnAreaOfEffectEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	// If we are currently tracking this actor, stop doing so
	if (_effectReapplyTimers.Contains(OtherActor))
		GetWorld()->GetTimerManager().ClearTimer(_effectReapplyTimers[OtherActor]);
}
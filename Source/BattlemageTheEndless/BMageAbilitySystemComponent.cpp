// Fill out your copyright notice in the Description page of Project Settings.


#include "BMageAbilitySystemComponent.h"
using namespace std::chrono;

UBMageAbilitySystemComponent::UBMageAbilitySystemComponent()
{
	_markSphere = CreateDefaultSubobject<USphereComponent>(TEXT("MarkSphere"));
	_markSphere->OnComponentBeginOverlap.AddDynamic(this, &UBMageAbilitySystemComponent::OnMarkSphereBeginOverlap);
	_markSphere->OnComponentEndOverlap.AddDynamic(this, &UBMageAbilitySystemComponent::OnMarkSphereEndOverlap);
	_markSphere->SetCollisionResponseToAllChannels(ECR_Overlap);

	ComboManager = CreateDefaultSubobject<UAbilityComboManager>(TEXT("ComboManager"));

	ProjectileManager = CreateDefaultSubobject<UProjectileManager>(TEXT("ProjectileManager"));
	EnsureInitSubObjects();
}

void UBMageAbilitySystemComponent::DeactivatePickup(APickupActor* pickup)
{
	ActivePickups.Remove(pickup);

	_removePickupFromAbilityByRangeCache(pickup);
}

void UBMageAbilitySystemComponent::ActivatePickup(APickupActor* pickup) 
{
	// The handles in GAS change each time we grant an ability, so we need to reset them each time we equip a new weapon
	// TODO: We should be able to assign all abilities on begin play and not have to do this since we're explicit when activating an ability
	if (ComboManager->Combos.Contains(pickup))
		ComboManager->Combos.Remove(pickup);

	// grant abilities for the new weapon
	// Needs to happen before bindings since bindings look up assigned abilities
	for (TSubclassOf<UGameplayAbility>& ability : pickup->Weapon->GrantedAbilities)
	{
		FGameplayAbilitySpecHandle handle = GiveAbility(FGameplayAbilitySpec(ability, 1, static_cast<int32>(EGASAbilityInputId::Confirm), pickup));
		ComboManager->AddAbilityToCombo(pickup, ability->GetDefaultObject<UGA_WithEffectsBase>(), handle);
	}

	ActivePickups.Add(pickup);
}

bool UBMageAbilitySystemComponent::GetShouldTick() const
{
	if (auto attributes = GetAttributeSet(UBaseAttributeSet::StaticClass()))
	{
		auto baseAttributes = Cast<UBaseAttributeSet>(attributes);
		if (baseAttributes && baseAttributes->GetHealthRegenRate() > 0.0f && baseAttributes->GetHealth() < baseAttributes->GetMaxHealth())
		{
			return true;
		}
	}

	return Super::GetShouldTick();
}

void UBMageAbilitySystemComponent::EnsureInitSubObjects()
{
	// This is kind of a hack, for AI the attributes are getting set in the CTOR but somehow becoming null by BeginPlay
	if (!ComboManager)
		ComboManager = NewObject<UAbilityComboManager>(this, TEXT("ComboManager"));

	ComboManager->AbilitySystemComponent = this;

	AbilityFailedCallbacks.AddUObject(ComboManager, &UAbilityComboManager::OnAbilityFailed);

	if (!ProjectileManager)
	{
		ProjectileManager = NewObject<UProjectileManager>(this, TEXT("ProjectileManager"));
	}

	if (auto ownerCharacter = Cast<ACharacter>(GetOwnerActor()))
		ProjectileManager->Initialize(ownerCharacter);
}

void UBMageAbilitySystemComponent::_removePickupFromAbilityByRangeCache(UObject* pickup)
{
	// Remove pickup from all cached range entries
	for (auto& entry : _abilitiesByRangeCache)
	{
		if (entry.Value.Abilities.Contains(pickup))
		{
			entry.Value.Abilities.Remove(pickup);
		}
	}

	// Clean up any empty entries
	_abilitiesByRangeCache = _abilitiesByRangeCache.FilterByPredicate(
		[](const TPair<float, FAbilitiesByRangeCacheEntry>& pair) {
			return pair.Value.Abilities.Num() > 0;
		}
	);
}

TArray<FAbilitiesByRangeCacheEntryDetail> UBMageAbilitySystemComponent::_getCachedAbilitiesInRange(float range, UObject* sourceObject)
{
	if (_abilitiesByRangeCache.Contains(range) && _abilitiesByRangeCache[range].Abilities.Contains(sourceObject))
	{
		return _abilitiesByRangeCache[range].Abilities[sourceObject];
	}
	else if (_abilitiesByRangeCache.Contains(range) && !sourceObject)
	{
		// Return all cached abilities for this range if a pickup is not specified
		TArray<FAbilitiesByRangeCacheEntryDetail> returnAbilities;
		for (auto& pair : _abilitiesByRangeCache[range].Abilities)
		{
			returnAbilities.Append(pair.Value);
		}
		return returnAbilities;
	}

	return TArray<FAbilitiesByRangeCacheEntryDetail>();
}

void UBMageAbilitySystemComponent::BeginPlay()
{
	Super::BeginPlay();
	// safety check since the bp editor can call this
	if (GetWorld())
	{
		_markSphere->SetVisibility(true);
		_markSphere->SetHiddenInGame(true);
		_markSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		_markSphere->SetGenerateOverlapEvents(false);
		//_markSphere->RegisterComponent();
		_markSphere->AttachToComponent(GetOwner()->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
	}

	EnsureInitSubObjects();

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

void UBMageAbilitySystemComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	auto attrs = Cast<UBaseAttributeSet>(GetAttributeSet(UBaseAttributeSet::StaticClass()));
	if (!attrs)
		return;

	if (attrs->GetHealthRegenRate() > 0.0f && attrs->GetHealth() < attrs->GetMaxHealth())
	{
		float AmountToAdd = FMath::Clamp(attrs->GetHealthRegenRate() * DeltaTime, 0.f, attrs->GetMaxHealth() - attrs->GetHealth());
		auto specHandle = MakeOutgoingSpec(UGE_RegenHealth::StaticClass(), 1.f, MakeEffectContext());

		if (specHandle.IsValid())
		{
			specHandle.Data->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(FName("Attributes.HealthToRegen")), AmountToAdd);
			ApplyGameplayEffectSpecToSelf(*specHandle.Data.Get());
			attrs->OnAttributeChanged.Broadcast(attrs->GetHealthAttribute(), attrs->GetHealth() - AmountToAdd, attrs->GetHealth());
		}
	}
}

TArray<FAbilitiesByRangeCacheEntryDetail> UBMageAbilitySystemComponent::GetAbilitiesInRange(float range, UObject* sourceObject)
{
	// If we've already cached abilities for this range and pickup, use those
	auto cached = _getCachedAbilitiesInRange(range, sourceObject);
	if (cached.Num() > 0)
	{
		return cached;
	}

	// Otherwise start from all activatable abilities, then filter down by range and pickup if provided
	TArray<FGameplayAbilitySpecHandle> AbilitiesInRange;
	auto abilities = GetActivatableAbilities();

	_abilitiesByRangeCache.Add(range, FAbilitiesByRangeCacheEntry());
	for (APickupActor* activePickup: ActivePickups)
	{
		_abilitiesByRangeCache[range].Abilities.Add(activePickup, TArray<FAbilitiesByRangeCacheEntryDetail>());
	}

	// Process all eligible abilities
	for (const FGameplayAbilitySpec& Spec : abilities)
	{
		if (!Spec.Ability)
			continue;

		// This can be null and is a supported use case for abilites granted directly to the character
		auto resolvedSource = Spec.SourceObject.Get();
		float abilityRange = GetAbilityRange(Spec.Ability);
		if (abilityRange <= range)
		{
			AbilitiesInRange.Add(Spec.Handle);
			FAbilitiesByRangeCacheEntryDetail entryDetail;
			entryDetail.Handle = Spec.Handle;
			entryDetail.Range = abilityRange;
			entryDetail.AttackType = EAttackType::Light; // TODO: Fix this
			_abilitiesByRangeCache[range].Abilities[resolvedSource].Add(entryDetail);
			continue;
		}
	}

	return _getCachedAbilitiesInRange(range, sourceObject);
}

float UBMageAbilitySystemComponent::GetAbilityRange(UGameplayAbility* ability)
{
	auto abilityWithRange = Cast<UGA_WithEffectsBase>(ability);
	// add the ability to the return list and cache
	if (abilityWithRange && abilityWithRange->MaxRange)
	{
		return abilityWithRange->MaxRange;
	}
	// assume abilities using the base UGameplayAbility class have infinite range
	return 9999999999.f;
}

FGameplayAbilitySpecHandle UBMageAbilitySystemComponent::GetLongestRangeAbilityWithinRange(float range, UObject* sourceObject)
{
	// TODO: Sort the cache when it's built to avoid this linear search
	float maxRangeFound = -1.f;
	FGameplayAbilitySpecHandle bestHandle;

	for (auto cacheEntryDetail : GetAbilitiesInRange(range, sourceObject))
	{
		if (cacheEntryDetail.Range > maxRangeFound)
		{
			maxRangeFound = cacheEntryDetail.Range;
			bestHandle = cacheEntryDetail.Handle;
		}
	}
	return bestHandle;
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
	/*if (GEngine)
		GEngine->AddOnScreenDebugMessage(-1, 1.5f, FColor::Yellow, FString::Printf(TEXT("Effect %s removed"), *EffectRemoved.Spec.Def->GetName()));*/
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

UAttackBaseGameplayAbility* UBMageAbilitySystemComponent::ProcessInputAndBindAbilityCancelled(APickupActor* PickupActor, EAttackType AttackType)
{
	// Let combo manager resolve and dispatch ability as needed
	auto specHandle = ComboManager->ProcessInput(PickupActor, AttackType);
	if (!specHandle.IsValid())
		return nullptr;

	// find the ability instance
	auto spec = FindAbilitySpecFromHandle(specHandle);
	if (!spec)
		return nullptr;

	auto instance = spec->GetPrimaryInstance();
	if (!instance)
		return nullptr;

	if (auto attackAbility = Cast<UAttackBaseGameplayAbility>(instance))
	{
		PostAbilityActivation(attackAbility);
		return attackAbility;
	}
	return nullptr;
}

bool UBMageAbilitySystemComponent::CancelAbilityByOwnedTag(FGameplayTag abilityTag)
{
	auto ability = GetActivatableAbilityByOwnedTag(abilityTag.GetTagName());
	if (!ability || !ability->IsActive())
		return false;

	CancelAbility(ability);
	return true;
}

APickupActor* UBMageAbilitySystemComponent::GetActivePickup(EquipSlot Slot)
{
	return *ActivePickups.FindByPredicate(
		[Slot](APickupActor* pickup) {
			return pickup->Weapon->SlotType == Slot;
		}
	);
}

void UBMageAbilitySystemComponent::PostAbilityActivation(UAttackBaseGameplayAbility* ability)
{
	// If the ability has projectiles to spawn, spawn them and exit
	//	Currently an ability can apply effects either on hit or to self, but not both
	if (ability->HitType == HitType::Projectile && ability->ProjectileConfiguration.ProjectileClass)
	{
		HandleProjectileSpawn(ability);

		// for charge spells we need to end the ability after the hit check
		if (ability->ChargeDuration > 0.001f)
			ability->EndSelf();

		return;
	}
	else if (ability->HitType == HitType::HitScan)
	{
		HandleHitScan(ability);

		// for charge spells we need to end the ability after the hit check
		if (ability->ChargeDuration > 0.001f)
			ability->EndSelf();

		return;
	}

	// If there are no effects, we have no need for a cancel callback
	if (ability->EffectsToApply.Num() == 0)
		return;

	// Otherwise bind the cancel event to remove the active effects immediately
	OnAbilityEnded.AddUObject(this, &UBMageAbilitySystemComponent::OnAbilityCancelled);
}

// TODO: Why is this in character instead of the ability?
void UBMageAbilitySystemComponent::HandleProjectileSpawn(UAttackBaseGameplayAbility* ability)
{
	auto ownerCharacter = Cast<ACharacter>(GetOwnerActor());

	// I'm unclear on why, but the address of the character seems to change at times
	if (!ProjectileManager->OwnerCharacter)
	{
		if (!ownerCharacter)
		{
			UE_LOG(LogExec, Error, TEXT("'%s' ProjectileManager has no valid OwnerCharacter!"), *GetNameSafe(this));
			return;
		}

		ProjectileManager->OwnerCharacter = ownerCharacter;
	}

	TArray<ABattlemageTheEndlessProjectile*> projectiles;

	// if the projectiles are spawned from an actor, use that entry point
	if (ability->ProjectileConfiguration.SpawnLocation == FSpawnLocation::Player)
	{
		projectiles = ProjectileManager->SpawnProjectiles_Actor(ability, ability->ProjectileConfiguration, nullptr);
	}
	// We can only spawn at last ability location if we have a niagara instance
	//  TODO: support spawning projectiles based on a previous ability's actor(s) as well
	else if (ability->ProjectileConfiguration.SpawnLocation == FSpawnLocation::SpellFocus)
	{
		auto controller = Cast<APlayerController>(ownerCharacter->GetController());
		const FRotator spawnRotation = controller ? controller->PlayerCameraManager->GetCameraRotation() : ownerCharacter->GetTransform().Rotator();
		auto socketName = IsLeftHanded ? FName("gripRight") : FName("gripLeft");
		auto activeSpellClass = GetActivePickup(EquipSlot::Secondary);
		if (!activeSpellClass)
		{
			UE_LOG(LogExec, Error, TEXT("'%s' Attempted to spawn projectiles from SpellFocus but has no active spell class!"), *GetNameSafe(this));
			return;

		}
		auto spawnLocation = ownerCharacter->GetMesh()->GetSocketLocation(socketName) + spawnRotation.RotateVector(activeSpellClass->Weapon->MuzzleOffset);

		// We are making the potentially dangerous assumption that there is only 1 instance
		projectiles = ProjectileManager->SpawnProjectiles_Location(ability, ability->ProjectileConfiguration,
			spawnRotation, spawnLocation, FVector::OneVector, activeSpellClass);
	}
	else if (ability->ProjectileConfiguration.SpawnLocation == FSpawnLocation::PreviousAbility)
	{
		if (ComboManager->LastAbilityNiagaraInstance)
		{
			// We are making the potentially dangerous assumption that there is only 1 instance
			projectiles = ProjectileManager->SpawnProjectiles_Location(
				ability, ability->ProjectileConfiguration, ComboManager->LastAbilityNiagaraInstance->GetComponentRotation(),
				ComboManager->LastAbilityNiagaraInstance->GetComponentLocation());
		}
	}
	else
	{
		UE_LOG(LogExec, Error, TEXT("'%s' Attempted to spawn projectiles from an invalid location!"), *GetNameSafe(this));
	}

	for (auto projectile : projectiles)
	{
		ownerCharacter->GetCapsuleComponent()->IgnoreActorWhenMoving(projectile, true);
		projectile->GetCollisionComp()->OnComponentHit.AddDynamic(ability, &UAttackBaseGameplayAbility::OnHit);
	}

}

void UBMageAbilitySystemComponent::HandleHitScan(UAttackBaseGameplayAbility* ability)
{
	auto ownerCharacter = Cast<ACharacter>(GetOwnerActor());

	// perform a line trace to determine if the ability hits anything
	// Uses camera socket as the start location
	FVector startLocation = ownerCharacter->GetMesh()->GetSocketLocation(FName("cameraSocket"));

	// Figure out the end location based on the ability's max range and current look direction
	UCharacterMovementComponent* movement = ownerCharacter->GetCharacterMovement();
	auto rotation = movement->GetLastUpdateRotation();

	if (ownerCharacter->IsPlayerControlled()) {
		rotation.Pitch = Cast<APlayerController>(ownerCharacter->Controller)->PlayerCameraManager->GetCameraRotation().Pitch;
		// TODO: Figure out what this needs to be instead for AI
	}

	FVector endLocation = startLocation + (rotation.Vector() * ability->MaxRange);

	//DrawDebugLine(GetWorld(), startLocation, endLocation, FColor::Red, false, 5.0f, 0, 1.0f);

	auto params = FCollisionQueryParams(FName(TEXT("LineTrace")), true, ownerCharacter);
	FHitResult hit = Traces::LineTraceGeneric(GetWorld(), params, startLocation, endLocation);

	auto hitActor = hit.GetActor();
	if (!hitActor)
		return;

	ACharacter* hitCharacter = Cast<ACharacter>(hit.GetActor());
	// if we hit something other than a character and this is a chain attack, render the hit at that location
	if (hitActor && !hitCharacter && ability->ChainSystem)
	{
		// We don't need to keep the reference, UE will handle disposing when it is destroyed
		auto chainEffectActor = GetWorld()->SpawnActor<AHitScanChainEffect>(AHitScanChainEffect::StaticClass(),
			ownerCharacter->GetActorLocation(), FRotator::ZeroRotator);
		chainEffectActor->Init(chainEffectActor, hitActor, ability->ChainSystem, hit.Location);
		return;
	}

	// if we hit a character apply effects to them (and chain if needed)
	TArray<ACharacter*> hitCharacters = { hitCharacter };
	if (ability->NumberOfChains > 0)
	{
		hitCharacters.Append(GetChainTargets(ability->NumberOfChains, ability->ChainDistance, hitCharacter));
	}

	if (ability->EffectsToApply.Num() == 0)
		return;

	// Handle chain delay if needed
	// floating point precision, woo
	if (FMath::Abs(ability->ChainDelay) < 0.00001f)
	{
		if (hitCharacters.Num() != 0)
		{
			for (auto applyTo : hitCharacters)
			{
				if (auto asc = applyTo->FindComponentByClass<UBMageAbilitySystemComponent>())
					ability->ApplyEffects(applyTo, asc, ownerCharacter);
			}
		}
	}
	else
	{
		for (int i = 0; i < hitCharacters.Num(); ++i)
		{
			auto asc = hitCharacters[i]->FindComponentByClass<UBMageAbilitySystemComponent>();
			// If the chain target doesn't have an ASC we can't apply effects to them
			if (!asc)
				continue;

			// No delay for the first target
			if (i == 0)
			{
				ability->ApplyEffects(hitCharacters[i], asc, ownerCharacter);
				continue;
			}

			// NOTE: Apparently this binding is not smart enough to handle default params, automatic casting of nullptrs, or downcasting so we need to do all of those manually
			FTimerDelegate TimerDelegate = FTimerDelegate::CreateUObject(
				ability,
				&UAttackBaseGameplayAbility::ApplyChainEffects,
				(AActor*)hitCharacters[i],
				(UAbilitySystemComponent*)asc,
				(AActor*)hitCharacters[i - 1],
				(AActor*)nullptr,
				i == ability->NumberOfChains
			);

			GetWorld()->GetTimerManager().SetTimer(ability->GetChainTimerHandles()[i - 1], TimerDelegate, ability->ChainDelay * i, false);
		}
	}
}


TArray<ACharacter*> UBMageAbilitySystemComponent::GetChainTargets(int NumberOfChains, float ChainDistance, ACharacter* HitActor)
{
	auto ownerCharacter = Cast<ACharacter>(GetOwnerActor());
	auto allBMages = TArray<AActor*>();
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ACharacter::StaticClass(), allBMages);

	// only keep potential targets which have an ASC
	allBMages = allBMages.FilterByPredicate(
		[](const AActor* a) {
			return a->FindComponentByClass<UBMageAbilitySystemComponent>() != nullptr;
		});

	auto theoreticalMaxDistance = NumberOfChains * ChainDistance;

	// Filter down to only the BMages within the theoreticalMaxDistance
	allBMages = allBMages.FilterByPredicate(
		[&HitActor, theoreticalMaxDistance, ownerCharacter](const AActor* a) {
			return a != ownerCharacter && a->GetDistanceTo(HitActor) <= theoreticalMaxDistance;
		});

	// If there's nothing to chain to, return an empty array
	if (allBMages.Num() == 0)
		return TArray<ACharacter*>();

	int remainingChains = NumberOfChains;
	auto chainTargets = TArray<ACharacter*>();
	ACharacter* nextChainTarget = HitActor;
	while (remainingChains > 0)
	{
		nextChainTarget = GetNextChainTarget(ChainDistance, nextChainTarget, allBMages);
		// If we couldn't find another eligible target, break
		if (!nextChainTarget)
			break;

		chainTargets.Add(nextChainTarget);

		--remainingChains;
	}

	return chainTargets;
}

ACharacter* UBMageAbilitySystemComponent::GetNextChainTarget(float ChainDistance, AActor* ChainActor, TArray<AActor*> Candidates)
{
	// If it's 0 we shouldn't have gotten here, if it's 1 then only ChainActor was passed
	if (Candidates.Num() <= 1)
		return nullptr;

	Candidates.Sort([ChainActor](const AActor& a, const AActor& b) {
		return a.GetDistanceTo(ChainActor) < b.GetDistanceTo(ChainActor);
		});

	for (auto actor : Candidates)
	{
		if (actor == ChainActor)
			continue;

		// check for line of sight
		// TODO: Use a specific trace channel rather than all of them
		auto params = FCollisionQueryParams(FName(TEXT("LineTrace")), true, ChainActor);
		auto hitResult = Traces::LineTraceGeneric(GetWorld(), params, ChainActor->GetActorLocation(), actor->GetActorLocation());
		auto hitActor = hitResult.GetActor();
		if (!hitActor || hitActor != actor)
			continue;

		// If we've passed the checks, this is the best candidate
		return Cast<ACharacter>(actor);
	}

	// we will only hit this if none of the candidates are in line of sight
	return nullptr;
}

void UBMageAbilitySystemComponent::OnAbilityCancelled(const FAbilityEndedData& endData)
{
	if (!endData.bWasCancelled)
		return;

	auto ability = Cast<UGA_WithEffectsBase>(FindAbilitySpecFromHandle(endData.AbilitySpecHandle)->Ability);
	if (!ability)
	{
		// if it's not our implementation with effects, there's nothing to do
		return;
	}

	for (TSubclassOf<UGameplayEffect> effect : ability->EffectsToApply)
	{
		if (effect->GetDefaultObject<UGameplayEffect>()->DurationPolicy != EGameplayEffectDurationType::HasDuration)
			continue;

		RemoveActiveGameplayEffectBySourceEffect(effect, this, 1);
	}
}

void UBMageAbilitySystemComponent::SuspendHealthRegen(float SuspendDurationOverride)
{
	// apply a zduration effect that sets health regen to 0
	auto specHandle = MakeOutgoingSpec(UGE_SuspendRegenHealth::StaticClass(), 1.f, MakeEffectContext());
	specHandle.Data->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(FName("Attributes.HealthRegenRate")), 0.0f);
	specHandle.Data->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(FName("Attributes.SuspendRegenDuration")), 
		SuspendDurationOverride == 0.f ? SuspendRegenOnHitDuration : SuspendDurationOverride);

	ApplyGameplayEffectSpecToSelf(*specHandle.Data.Get());
}

UAttackBaseGameplayAbility* UBMageAbilitySystemComponent::BeginChargeAbility(TSubclassOf<UGameplayAbility> InAbilityClass)
{
	auto abilitySpec = FindAbilitySpecFromClass(InAbilityClass);
	if (!abilitySpec)
	{
		UE_LOG(LogTemp, Warning, TEXT("No ability found of class %s"), *InAbilityClass->GetName());
		return nullptr;
	}
	return BeginChargeAbility_Internal(abilitySpec);
}

UAttackBaseGameplayAbility* UBMageAbilitySystemComponent::BeginChargeAbility_Tags(FGameplayTagContainer InAbilityTags)
{
	TArray<FGameplayAbilitySpecHandle> abilityHandles;
	FindAllAbilitiesWithTags(abilityHandles, InAbilityTags);

	if (abilityHandles.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("No ability found with tags %s"), *InAbilityTags.ToString());
		return nullptr;
	}
	if (abilityHandles.Num() > 1)
	{
		UE_LOG(LogTemp, Warning, TEXT("Multiple abilities found with tags %s, using first"), *InAbilityTags.ToString());
	}
	auto spec = FindAbilitySpecFromHandle(abilityHandles[0]);
	if (!spec || !spec->Ability)
	{
		UE_LOG(LogTemp, Warning, TEXT("Ability spec invalid for tags %s"), *InAbilityTags.ToString());
		return nullptr;
	}
	
	return BeginChargeAbility_Internal(spec);
}

UAttackBaseGameplayAbility* UBMageAbilitySystemComponent::BeginChargeAbility_Internal(FGameplayAbilitySpec* AbilitySpec)
{
	TObjectPtr<UGameplayAbility> activeInstance = AbilitySpec->GetPrimaryInstance();
	auto attackAbility = Cast<UAttackBaseGameplayAbility>(activeInstance);

	bool success = TryActivateAbility(AbilitySpec->Handle, true);
	if (!success && GEngine)
		GEngine->AddOnScreenDebugMessage(-1, 1.5f, FColor::Red, FString::Printf(TEXT("Ability %s failed to activate"), *AbilitySpec->Ability->GetName()));

	FTimerDelegate TimerDelegate = FTimerDelegate::CreateUObject(this, &UBMageAbilitySystemComponent::ChargeSpell);
	GetWorld()->GetTimerManager().SetTimer(_chargeSpellTimerHandle, TimerDelegate, 1.0f / (float)ChargeTickRate, true);

	_chargingAbility = attackAbility;

	if (GEngine)
		GEngine->AddOnScreenDebugMessage(-1, 1.5f, FColor::Yellow, FString::Printf(TEXT("Ability %s is charging"), *AbilitySpec->Ability->GetName()));

	return _chargingAbility;
}

void UBMageAbilitySystemComponent::ChargeSpell()
{
	if (!_chargingAbility || !_chargingAbility->IsValidLowLevel())
	{
		UE_LOG(LogTemp, Warning, TEXT("No charging ability to progress"));
		CompleteChargeAbility();
		return;
	}

	bool isChargedBefore = _chargingAbility->IsCharged();
	_chargingAbility->HandleChargeProgress();
	if (!isChargedBefore && _chargingAbility->IsCharged() && GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 1.5f, FColor::Yellow, FString::Printf(TEXT("Ability %s is charged"), *_chargingAbility->GetName()));
	}
}

void UBMageAbilitySystemComponent::CompleteChargeAbility()
{
	GetWorld()->GetTimerManager().ClearTimer(_chargeSpellTimerHandle);

	if (!_chargingAbility || !_chargingAbility->IsValidLowLevel())
	{
		UE_LOG(LogTemp, Warning, TEXT("No charging ability to complete"));
		return;	
	}

	// if charge spell and completed event, clear the charge timer
	if (_chargingAbility->CastSound)
	{
		UGameplayStatics::SpawnSoundAttached(_chargingAbility->CastSound, GetOwnerActor()->GetRootComponent(),
			// The next two lines are all default values, just making them explicit to get to the attenuation parameter
			NAME_None, FVector(ForceInit), FRotator::ZeroRotator, EAttachLocation::KeepRelativeOffset, false,
			1.f, 1.f, 0.f,
			_chargingAbility->CastSoundAttenuation);
	}

	PostAbilityActivation(_chargingAbility);

	// TODO: Is this needed? PostAbilityActivation handles it already
	// This is a hack to call EndAbility without needing to spoof up all the params
	_chargingAbility->OnMontageCompleted();
}
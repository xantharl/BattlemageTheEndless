// Fill out your copyright notice in the Description page of Project Settings.


#include "SwarmManagerComponent.h"

#include "BattlemageTheEndlessCharacter.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "GameFramework/GameMode.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

#if !UE_BUILD_SHIPPING
static TAutoConsoleVariable<int32> CVarSwarmAggroDebug(
	TEXT("swarm.AggroDebug"),
	0,
	TEXT("Visualize SwarmManager aggro tables. 0 = off, 1 = on."),
	ECVF_Cheat);
#endif

// Sets default values for this component's properties
USwarmManagerComponent::USwarmManagerComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
}


// Called when the game starts
void USwarmManagerComponent::BeginPlay()
{
	Super::BeginPlay();
	auto Owner = GetOwner();
	if (!Owner) return;

	auto GameMode = Cast<AGameModeBase>(Owner);
	if (!GameMode) return;


}

void USwarmManagerComponent::RegisterEnemy(AActor* Enemy)
{
	if (!Enemy) return;
	if (ManagedActors.Contains(Enemy)) return;

	ManagedActors.Add(Enemy);
	AggroTables.FindOrAdd(Enemy);
}

void USwarmManagerComponent::UnregisterEnemy(AActor* Enemy)
{
	if (!Enemy) return;

	ManagedActors.Remove(Enemy);
	AggroTables.Remove(Enemy);

	for (TPair<AActor*, FAggroTable>& Pair : AggroTables)
	{
		Pair.Value.AggroByTargetActor.Remove(Enemy);
		if (Pair.Value.HighestThreatActor == Enemy)
		{
			Pair.Value.HighestThreatActor = nullptr;
			HighestAggroChanged.Broadcast(Pair.Key, nullptr);
		}
	}
}

void USwarmManagerComponent::AddAggro(AActor* Victim, AActor* Attacker, float Amount)
{
	if (!Victim || !Attacker) return;
	if (Victim == Attacker) return;
	if (Amount <= 0.f) return;
	if (!ManagedActors.Contains(Victim)) return;

	// Cancel any pending or active decay for this pair so a fresh hit fully resets the decay clock.
	ActiveDecayRules.RemoveAll(
		[Victim, Attacker](const FAggroDecayRule& R) { return R.Enemy == Victim && R.Target == Attacker; });

	FAggroTable& Table = AggroTables.FindOrAdd(Victim);
	float& Current = Table.AggroByTargetActor.FindOrAdd(Attacker);
	Current += Amount;

	const float HighestAggro = Table.HighestThreatActor
		? Table.AggroByTargetActor.FindRef(Table.HighestThreatActor)
		: -TNumericLimits<float>::Max();

	if (Current > HighestAggro)
	{
		Table.HighestThreatActor = Attacker;
		HighestAggroChanged.Broadcast(Victim, Attacker);
	}
}

void USwarmManagerComponent::RemoveAggroForTarget(AActor* Enemy, AActor* Target)
{
	if (!Enemy || !Target) return;

	FAggroTable* Table = AggroTables.Find(Enemy);
	if (!Table) return;

	if (Table->AggroByTargetActor.Remove(Target) == 0) return;
	if (Table->HighestThreatActor != Target) return; // top didn't change

	AActor* NewHighest = nullptr;
	float HighestValue = -TNumericLimits<float>::Max();
	for (const TPair<AActor*, float>& Pair : Table->AggroByTargetActor)
	{
		if (Pair.Value > HighestValue)
		{
			HighestValue = Pair.Value;
			NewHighest = Pair.Key;
		}
	}

	Table->HighestThreatActor = NewHighest;
	HighestAggroChanged.Broadcast(Enemy, NewHighest);
}

void USwarmManagerComponent::RemoveAggroFromAllTables(AActor* Target)
{
	if (!Target) return;

	for (TPair<AActor*, FAggroTable>& Pair : AggroTables)
	{
		RemoveAggroForTarget(Pair.Key, Target);
	}
}

void USwarmManagerComponent::DecayAggro(AActor* Enemy, AActor* Target, float DecayRatePerSecond, float Delay)
{
	if (!Enemy || !Target) return;

	const int32 ExistingIndex = ActiveDecayRules.IndexOfByPredicate(
		[Enemy, Target](const FAggroDecayRule& R) { return R.Enemy == Enemy && R.Target == Target; });

	if (DecayRatePerSecond <= 0.f)
	{
		if (ExistingIndex != INDEX_NONE) ActiveDecayRules.RemoveAt(ExistingIndex);
		return;
	}

	const float ClampedDelay = FMath::Max(0.f, Delay);

	if (ExistingIndex != INDEX_NONE)
	{
		ActiveDecayRules[ExistingIndex].RatePerSecond = DecayRatePerSecond;
		ActiveDecayRules[ExistingIndex].DelayRemaining = ClampedDelay;
		return;
	}

	ActiveDecayRules.Add({ Enemy, Target, DecayRatePerSecond, ClampedDelay });
}

void USwarmManagerComponent::TickAggroDecay(float DeltaTime)
{
	for (int32 i = ActiveDecayRules.Num() - 1; i >= 0; --i)
	{
		FAggroDecayRule& Rule = ActiveDecayRules[i];
		if (!Rule.Enemy || !Rule.Target)
		{
			ActiveDecayRules.RemoveAt(i);
			continue;
		}

		if (Rule.DelayRemaining > 0.f)
		{
			Rule.DelayRemaining -= DeltaTime;
			continue;
		}

		FAggroTable* Table = AggroTables.Find(Rule.Enemy);
		if (!Table) continue;
		float* Aggro = Table->AggroByTargetActor.Find(Rule.Target);
		if (!Aggro) continue;

		*Aggro -= Rule.RatePerSecond * DeltaTime;

		if (*Aggro <= 0.f)
		{
			AActor* DecayedEnemy = Rule.Enemy;
			AActor* DecayedTarget = Rule.Target;
			ActiveDecayRules.RemoveAt(i);
			RemoveAggroForTarget(DecayedEnemy, DecayedTarget);
			continue;
		}

		// Still positive — if the decaying target was the top, the leaderboard may have changed.
		if (Table->HighestThreatActor != Rule.Target) continue;

		AActor* NewHighest = nullptr;
		float HighestValue = -TNumericLimits<float>::Max();
		for (const TPair<AActor*, float>& Pair : Table->AggroByTargetActor)
		{
			if (Pair.Value > HighestValue)
			{
				HighestValue = Pair.Value;
				NewHighest = Pair.Key;
			}
		}

		if (NewHighest != Table->HighestThreatActor)
		{
			Table->HighestThreatActor = NewHighest;
			HighestAggroChanged.Broadcast(Rule.Enemy, NewHighest);
		}
	}
}

USwarmManagerComponent* USwarmManagerComponent::Get(const UObject* WorldContextObject)
{
	if (!WorldContextObject) return nullptr;

	UWorld* World = WorldContextObject->GetWorld();
	if (!World) return nullptr;

	AGameModeBase* GameMode = World->GetAuthGameMode();
	if (!GameMode) return nullptr;

	return GameMode->FindComponentByClass<USwarmManagerComponent>();
}


// Called every frame
void USwarmManagerComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	UpdatePlayerSlotLocations();
	TickAggroDecay(DeltaTime);

#if !UE_BUILD_SHIPPING
	if (CVarSwarmAggroDebug.GetValueOnGameThread() != 0)
	{
		DrawAggroDebug();
	}
#endif
}

#if !UE_BUILD_SHIPPING
void USwarmManagerComponent::DrawAggroDebug() const
{
	UWorld* World = GetWorld();
	if (!World) return;

	for (const TPair<AActor*, FAggroTable>& EnemyEntry : AggroTables)
	{
		AActor* Enemy = EnemyEntry.Key;
		if (!Enemy) continue;

		const FAggroTable& Table = EnemyEntry.Value;
		if (Table.AggroByTargetActor.Num() == 0) continue;

		float MaxAggro = 0.f;
		for (const TPair<AActor*, float>& T : Table.AggroByTargetActor)
		{
			MaxAggro = FMath::Max(MaxAggro, T.Value);
		}
		if (MaxAggro <= 0.f) continue;

		const FVector EnemyLoc = Enemy->GetActorLocation() + FVector(0.f, 0.f, 50.f);

		FString Label = FString::Printf(TEXT("%s"), *Enemy->GetName());
		for (const TPair<AActor*, float>& Pair : Table.AggroByTargetActor)
		{
			AActor* Target = Pair.Key;
			if (!Target) continue;

			const bool bIsTop = (Target == Table.HighestThreatActor);
			const float Ratio = FMath::Clamp(Pair.Value / MaxAggro, 0.f, 1.f);
			const FColor LineColor = bIsTop
				? FColor::Red
				: FColor(static_cast<uint8>(255 * Ratio), static_cast<uint8>(255 * (1.f - Ratio)), 0);
			const float Thickness = bIsTop ? 2.5f : 1.f;

			DrawDebugLine(World, EnemyLoc, Target->GetActorLocation() + FVector(0.f, 0.f, 50.f),
				LineColor, false, 0.2f, 0, Thickness);

			Label += FString::Printf(TEXT("\n %s%s: %.1f"),
				bIsTop ? TEXT("* ") : TEXT("  "), *Target->GetName(), Pair.Value);
		}

		DrawDebugString(World, EnemyLoc + FVector(0.f, 0.f, 40.f), Label, nullptr, FColor::White, 0.2f, true);
	}
}
#endif

void USwarmManagerComponent::InitRelativePositions()
{
	RelativePositions.Reset();
	if (AmountOfSlotsPerPlayer <= 0) return;

	const double AngleStep = 360.0 / static_cast<double>(AmountOfSlotsPerPlayer);
	const FVector BaseVec(DistanceToMaintain, 0.0, 0.0);

	RelativePositions.Reserve(AmountOfSlotsPerPlayer);
	for (int32 i = 0; i < AmountOfSlotsPerPlayer; ++i)
	{
		const double AngleDeg = AngleStep * static_cast<double>(i);
		RelativePositions.Add(BaseVec.RotateAngleAxis(AngleDeg, FVector::UpVector));
	}
}

void USwarmManagerComponent::InitPlayerSwarmSlots(APawn* ControlledPawn)
{
	if (!ControlledPawn) return;

	const FVector PawnLoc = ControlledPawn->GetActorLocation();

	FPlayerSwarmSlots Entry;
	Entry.Player = ControlledPawn;
	Entry.SwarmSlots.Reserve(RelativePositions.Num());
	for (const FVector& Offset : RelativePositions)
	{
		FSwarmSlot Slot;
		Slot.Location = PawnLoc + Offset;
		Slot.Claimant = nullptr;
		Entry.SwarmSlots.Add(MoveTemp(Slot));
	}

	PlayerSwarmSlots.Add(MoveTemp(Entry));
}

void USwarmManagerComponent::UpdatePlayerSlotLocations()
{
	static const FName MoveToLocKey(TEXT("ClaimedMoveToLocation"));

	for (FPlayerSwarmSlots& Entry : PlayerSwarmSlots)
	{
		if (!Entry.Player) continue;
		if (Entry.SwarmSlots.Num() != RelativePositions.Num()) continue;

		const FVector PlayerLoc = Entry.Player->GetActorLocation();
		const FVector PlayerForwardOffset = Entry.Player->GetActorForwardVector() * 0.1;

		for (int32 i = 0; i < Entry.SwarmSlots.Num(); ++i)
		{
			FSwarmSlot& Slot = Entry.SwarmSlots[i];
			const FVector NewLoc = PlayerLoc + PlayerForwardOffset + RelativePositions[i];
			Slot.Location = NewLoc;

// #if !UE_BUILD_SHIPPING
// 			DrawDebugSphere(GetWorld(), NewLoc, 10.f, 12, FColor(1, 82, 0), false, 1.f, 0, 0.f);
// #endif

			if (!Slot.Claimant) continue;

			UBlackboardComponent* Blackboard = UAIBlueprintHelperLibrary::GetBlackboard(Slot.Claimant);
			if (!Blackboard) continue;

			Blackboard->SetValueAsVector(MoveToLocKey, NewLoc);
		}
	}
}

bool USwarmManagerComponent::TryClaimPosition(AActor* Requestor, FVector Position, double ToleranceRadius, AActor* RefActor)
{
	if (!Requestor) return false;

	// Reject if any *other* actor's claimed position is within tolerance of the requested position
	for (const TPair<AActor*, FVector>& Pair : ClaimedPositions)
	{
		if (Pair.Key == Requestor) continue;
		if (FVector::Dist(Pair.Value, Position) < ToleranceRadius)
		{
			return false;
		}
	}

	ClaimedPositions.Add(Requestor, Position);
	return true;
}

FSwarmSlot* USwarmManagerComponent::ClaimNearestAvailablePosition(AActor* TargetActor, AActor* Claimant, const FVector& RequestedLocation)
{
	if (!TargetActor || !Claimant) return nullptr;

	FPlayerSwarmSlots* Found = FindPlayerSwarmSlots(TargetActor);
	if (!Found) return nullptr;

	TArray<FSwarmSlot>& Slots = Found->SwarmSlots;

	int32 BestIndex = INDEX_NONE;
	double BestDistSq = TNumericLimits<double>::Max();
	for (int32 i = 0; i < Slots.Num(); ++i)
	{
		if (Slots[i].Claimant) continue; // already claimed
		const double DistSq = FVector::DistSquared(Slots[i].Location, RequestedLocation);
		if (DistSq < BestDistSq)
		{
			BestDistSq = DistSq;
			BestIndex = i;
		}
	}

	if (BestIndex == INDEX_NONE) return nullptr;

	Slots[BestIndex].Claimant = Claimant;
	return &Slots[BestIndex];
}

bool USwarmManagerComponent::ClaimNearestAvailablePosition(AActor* TargetActor, AActor* Claimant, FVector RequestedLocation, FSwarmSlot& OutClaimedSlot)
{
	if (FSwarmSlot* Claimed = ClaimNearestAvailablePosition(TargetActor, Claimant, RequestedLocation))
	{
		OutClaimedSlot = *Claimed;
		return true;
	}

	OutClaimedSlot = FSwarmSlot{};
	return false;
}

void USwarmManagerComponent::FreeClaimedPosition(AActor* Claimant)
{
	if (!Claimant) return;
	if (!ClaimedPositions.Contains(Claimant)) return;

	ClaimedPositions.Remove(Claimant);
}

void USwarmManagerComponent::SwitchTargets(AActor* NewTarget, AActor* Claimant)
{
	if (!NewTarget || !Claimant) return;

	UBlackboardComponent* Blackboard = UAIBlueprintHelperLibrary::GetBlackboard(Claimant);
	if (!Blackboard)
	{
		UE_LOG(LogTemp, Warning, TEXT("SwarmManager::SwitchTargets called on Actor with no blackboard"));
		return;
	}

	static const FName TargetActorKey(TEXT("TargetActor"));
	if (AActor* CurrentTarget = Cast<AActor>(Blackboard->GetValueAsObject(TargetActorKey)))
	{
		if (FPlayerSwarmSlots* Found = FindPlayerSwarmSlots(CurrentTarget))
		{
			RemoveSwarmSlotClaim(Found->SwarmSlots, Claimant);
		}
		FreeClaimedPosition(Claimant);
	}

	ClaimNearestAvailablePosition(NewTarget, Claimant, Claimant->GetActorLocation());

	Blackboard->SetValueAsObject(TargetActorKey, NewTarget);
}

void USwarmManagerComponent::RemoveSwarmSlotClaim(TArray<FSwarmSlot>& SwarmSlots, AActor* ClaimantToRemove)
{
	if (!ClaimantToRemove) return;

	for (FSwarmSlot& Slot : SwarmSlots)
	{
		if (Slot.Claimant == ClaimantToRemove)
		{
			Slot.Claimant = nullptr;
		}
	}
}

FPlayerSwarmSlots* USwarmManagerComponent::FindPlayerSwarmSlots(AActor* TargetActor)
{
	if (!TargetActor) return nullptr;

	return PlayerSwarmSlots.FindByPredicate(
		[TargetActor](const FPlayerSwarmSlots& Entry) { return Entry.Player == TargetActor; });
}

void USwarmManagerComponent::RestartPlayer(APlayerController* NewPlayer)
{
	if (!NewPlayer) return;

	APawn* NewPawn = NewPlayer->GetPawn();
	if (!NewPawn) return;

	if (RelativePositions.Num() == 0)
	{
		InitRelativePositions();
	}
	InitPlayerSwarmSlots(NewPawn);
}

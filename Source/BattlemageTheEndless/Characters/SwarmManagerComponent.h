// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SwarmManagerComponent.generated.h"

class ABattlemageTheEndlessCharacter;
class APawn;

/** Broadcast when the highest-aggro actor for an enemy changes. NewHighestThreat may be null. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnHighestAggroChanged, AActor*, Enemy, AActor*, NewHighestThreat);

/** Structure representing an Aggro table for a single AI enemy **/
USTRUCT(Blueprintable)
struct FAggroTable
{
	GENERATED_BODY()

public:
	/** Map of actors and associated aggro values, highest value has focus **/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aggro")
	TMap<AActor*, float> AggroByTargetActor;

	/** Current highest threat actor, maintained on aggro change **/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aggro")
	TObjectPtr<AActor> HighestThreatActor;
};

/** A single swarm slot around a player */
USTRUCT(BlueprintType)
struct FSwarmSlot
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Swarm")
	FVector Location = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Swarm")
	TObjectPtr<AActor> Claimant;
};

/** Active per-(Enemy, Target) aggro decay rule. Aggro drops by RatePerSecond each second until the entry hits 0. */
USTRUCT()
struct FAggroDecayRule
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<AActor> Enemy;

	UPROPERTY()
	TObjectPtr<AActor> Target;

	UPROPERTY()
	float RatePerSecond = 0.f;
};

/** Per-player array of swarm slots; last-tick world locations swarm members can claim */
USTRUCT(BlueprintType)
struct FPlayerSwarmSlots
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Swarm")
	TObjectPtr<AActor> Player;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Swarm")
	TArray<FSwarmSlot> SwarmSlots;
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), Blueprintable )
class BATTLEMAGETHEENDLESS_API USwarmManagerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	USwarmManagerComponent();
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Swarm Manager")
	TArray<AActor*> ManagedActors;

	// ** Begin Aggro Management ** //
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aggro")
	TMap<AActor*, FAggroTable> AggroTables;

	/** Fired when an enemy's HighestThreatActor changes (including when it becomes null). */
	UPROPERTY(BlueprintAssignable, Category = "Aggro")
	FOnHighestAggroChanged HighestAggroChanged;

	// ** End Aggro Management ** //

	// ** Begin Swarm Slot Configuration ** //

	/** Radius of the slot ring placed around each player */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Swarm Manager|Config")
	double DistanceToMaintain = 250.0;

	/** Number of slots distributed evenly around each player */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Swarm Manager|Config")
	int32 AmountOfSlotsPerPlayer = 5;

	// ** End Swarm Slot Configuration ** //

	// ** Begin Swarm Slot Runtime State ** //

	/** Map of claimant actor → claimed world position */
	UPROPERTY(BlueprintReadWrite, Category = "Swarm Manager|State")
	TMap<AActor*, FVector> ClaimedPositions;

	/** Players currently being tracked for swarm slot generation */
	UPROPERTY(BlueprintReadWrite, Category = "Swarm Manager|State")
	TArray<TObjectPtr<ABattlemageTheEndlessCharacter>> PlayerCharacters;

	/** Slot offsets relative to a player (computed once from DistanceToMaintain + AmountOfSlotsPerPlayer) */
	UPROPERTY(BlueprintReadWrite, Category = "Swarm Manager|State")
	TArray<FVector> RelativePositions;

	/** Per-player swarm slot world locations — refreshed every tick */
	UPROPERTY(BlueprintReadWrite, Category = "Swarm Manager|State")
	TArray<FPlayerSwarmSlots> PlayerSwarmSlots;

	// ** End Swarm Slot Runtime State ** //

	UFUNCTION(BlueprintCallable, Category = "Swarm Manager")
	void RegisterEnemy(AActor* Enemy);

	UFUNCTION(BlueprintCallable, Category = "Swarm Manager")
	void UnregisterEnemy(AActor* Enemy);

	/** Add aggro from Attacker on Victim. No-op unless Victim is in ManagedActors. */
	UFUNCTION(BlueprintCallable, Category = "Aggro")
	void AddAggro(AActor* Victim, AActor* Attacker, float Amount);

	/** Remove Target's entry from Enemy's aggro table. If Target was the top threat, recomputes and broadcasts HighestAggroChanged. */
	UFUNCTION(BlueprintCallable, Category = "Aggro")
	void RemoveAggroForTarget(AActor* Enemy, AActor* Target);

	/** Remove Target from every aggro table. Called when an actor dies so other AI stop tracking them. */
	UFUNCTION(BlueprintCallable, Category = "Aggro")
	void RemoveAggroFromAllTables(AActor* Target);

	/** Register an aggro decay rule for (Enemy, Target). Call again to update the rate. Pass <= 0 to clear. */
	UFUNCTION(BlueprintCallable, Category = "Aggro")
	void DecayAggro(AActor* Enemy, AActor* Target, float DecayRatePerSecond);

	/** Active per-pair aggro decay rules; processed in TickComponent. */
	UPROPERTY()
	TArray<FAggroDecayRule> ActiveDecayRules;

	UFUNCTION(BlueprintPure, Category = "Swarm Manager", meta = (WorldContext = "WorldContextObject"))
	static USwarmManagerComponent* Get(const UObject* WorldContextObject);

	// ** Begin Swarm Slot API ** //

	UFUNCTION(BlueprintCallable, Category = "Swarm Manager")
	void InitRelativePositions();

	UFUNCTION(BlueprintCallable, Category = "Swarm Manager")
	void InitPlayerSwarmSlots(APawn* ControlledPawn);

	UFUNCTION(BlueprintCallable, Category = "Swarm Manager")
	void UpdatePlayerSlotLocations();

	UFUNCTION(BlueprintCallable, Category = "Swarm Manager")
	bool TryClaimPosition(AActor* Requestor, FVector Position, double ToleranceRadius, AActor* RefActor);

	/** Claim the closest unclaimed slot in TargetActor's swarm. Returns the claimed slot, or nullptr if none was available. */
	FSwarmSlot* ClaimNearestAvailablePosition(AActor* TargetActor, AActor* Claimant, const FVector& RequestedLocation);

	/** BP-callable overload. Returns true on success and writes the claimed slot to OutClaimedSlot. */
	UFUNCTION(BlueprintCallable, Category = "Swarm Manager")
	bool ClaimNearestAvailablePosition(AActor* TargetActor, AActor* Claimant, FVector RequestedLocation, FSwarmSlot& OutClaimedSlot);

	UFUNCTION(BlueprintCallable, Category = "Swarm Manager")
	void FreeClaimedPosition(AActor* Claimant);

	UFUNCTION(BlueprintCallable, Category = "Swarm Manager")
	void SwitchTargets(AActor* NewTarget, AActor* Claimant);

	/** Returns a pointer to the FPlayerSwarmSlots entry for TargetActor, or nullptr if none. */
	FPlayerSwarmSlots* FindPlayerSwarmSlots(AActor* TargetActor);

	/** Re-init slots for the new player's pawn; idempotent for the relative-positions seed */
	UFUNCTION(BlueprintCallable, Category = "Swarm Manager")
	void RestartPlayer(APlayerController* NewPlayer);

	// ** End Swarm Slot API ** //

private:
	/** Clear Claimant on every slot in the array currently owned by ClaimantToRemove */
	void RemoveSwarmSlotClaim(TArray<FSwarmSlot>& SwarmSlots, AActor* ClaimantToRemove);

	/** Apply each active decay rule once for the given DeltaTime. */
	void TickAggroDecay(float DeltaTime);

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
};

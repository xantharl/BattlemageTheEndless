// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "NiagaraComponent.h"
#include "BattlemageTheEndless/Abilities/GA_WithEffectsBase.h"
#include "BattlemageTheEndless/Abilities/Combos/AbilityCombo.h"
#include "BattlemageTheEndless/Abilities/AttackBaseGameplayAbility.h"
#include "BattlemageTheEndless/Pickups/PickupActor.h"
#include "Components/ActorComponent.h"
#include "ComboManagerComponent.generated.h"

USTRUCT(BlueprintType)
struct FPickupCombos
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Category = Equipment, meta = (AllowPrivateAccess = "true"))
	TArray<UAbilityCombo*> Combos = TArray<UAbilityCombo*>();

	UPROPERTY(BlueprintReadOnly, Category = Equipment, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAbilityCombo> ActiveCombo;
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class BATTLEMAGETHEENDLESS_API UComboManagerComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UComboManagerComponent();
	
	FGameplayAbilityActorInfo GetOwnerActorInfo();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;
	FTimerHandle ComboTimerHandle;

	int CurrentComboNumber = 0;

	UFUNCTION()
	void ActivateAbilityAndResetTimer(FGameplayAbilitySpec abilitySpec);

	void EndComboHandler();

	FGameplayAbilitySpecHandle* SwitchAndAdvanceCombo(APickupActor* PickupActor, UAbilityCombo* Combo);

	bool CheckCooldownAndTryActivate(FGameplayAbilitySpec abilitySpec);

	/// <summary>
	/// The combo manager allows for an action queue of 1, this is used in the event that input
	///  is received for an ability while one is ongoing
	/// </summary>
	FGameplayAbilitySpecHandle* NextAbilityHandle;

	FTimerHandle QueuedAbilityTimer;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	
	UPROPERTY(BlueprintReadOnly, Category = Combo, meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UGameplayAbility> LastActivatedAbilityClass;

	// Is this actually in use?
	TObjectPtr<UNiagaraComponent> LastAbilityNiagaraInstance;

	void OnAbilityFailed(const UGameplayAbility* ability, const FGameplayTagContainer& reason);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Equipment, meta = (AllowPrivateAccess = "true"))
	TMap<APickupActor*, FPickupCombos> Combos;

	// Adds a single ability to a combo, or creates a new combo if the ability is the first in a combo
	void AddAbilityToCombo(APickupActor* PickupActor, UGA_WithEffectsBase* Ability, FGameplayAbilitySpecHandle Handle);

	/** Processes input from the player, can activate immediately or queue an action. Returns an invalid handle if no ability was activated. **/
	FGameplayAbilitySpecHandle ProcessInput_Legacy(APickupActor* PickupActor, EAttackType AttackType);

	FGameplayAbilitySpecHandle ProcessInput(APickupActor* PickupActor, EAttackType AttackType);
	
	FGameplayAbilitySpecHandle DelegateToWeapon(APickupActor* PickupActor, EAttackType AttackType);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Combo, meta = (AllowPrivateAccess = "true"))
	// one second in std::chrono::milliseconds
	double ComboExpiryTime = 1.0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Combo, meta = (AllowPrivateAccess = "true"))
	UAbilitySystemComponent* AbilitySystemComponent;

	UAbilityCombo* FindComboByTag(APickupActor* PickupActor, const FGameplayTag& ComboTag);

private:
	FGameplayAbilityActorInfo _ownerActorInfo = FGameplayAbilityActorInfo();
	
};

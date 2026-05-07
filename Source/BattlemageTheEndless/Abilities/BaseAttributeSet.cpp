// Fill out your copyright notice in the Description page of Project Settings.


#include "BaseAttributeSet.h"
#include "../Characters/SwarmManagerComponent.h"
#include "GameplayEffectExtension.h"
#include "Net/UnrealNetwork.h"

UBaseAttributeSet::UBaseAttributeSet()
{
	Health.SetBaseValue(100.f);
	Health.SetCurrentValue(100.f);

	MaxHealth.SetBaseValue(100.f);
	MaxHealth.SetCurrentValue(100.f);
	
	Mana.SetBaseValue(100.f);
	Mana.SetCurrentValue(100.f);

	MaxMana.SetBaseValue(100.f);
	MaxMana.SetCurrentValue(100.f);

	HealthRegenRate.SetBaseValue(0.f);
	HealthRegenRate.SetCurrentValue(0.f);

	MovementSpeed.SetBaseValue(600.f);
	MovementSpeed.SetCurrentValue(600.f);
	CrouchedSpeed.SetBaseValue(300.f);
	CrouchedSpeed.SetCurrentValue(300.f);

	DamageModifierPhysical_Outbound.SetBaseValue(1.f);
	DamageModifierPhysical_Outbound.SetCurrentValue(1.f);
	
	DamageModifierPhysical_Inbound.SetBaseValue(1.f);
	DamageModifierPhysical_Inbound.SetCurrentValue(1.f);
	
	DamageModifierFire_Outbound.SetBaseValue(1.f);
	DamageModifierFire_Outbound.SetCurrentValue(1.f);
	
	DamageModifierFire_Inbound.SetBaseValue(1.f);
	DamageModifierFire_Inbound.SetCurrentValue(1.f);
}

void UBaseAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(UBaseAttributeSet, Health, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UBaseAttributeSet, MaxHealth, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UBaseAttributeSet, Mana, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UBaseAttributeSet, MaxMana, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UBaseAttributeSet, HealthRegenRate, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UBaseAttributeSet, MovementSpeed, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UBaseAttributeSet, CrouchedSpeed, COND_None, REPNOTIFY_Always);
}

void UBaseAttributeSet::PreNetReceive()
{
	Super::PreNetReceive();
	for (TFieldIterator<FStructProperty> It(GetClass()); It; ++It)
	{
		if (It->Struct != FGameplayAttributeData::StaticStruct()) continue;
		const FGameplayAttributeData* Data = It->ContainerPtrToValuePtr<FGameplayAttributeData>(this);
		_preNetAttributeValues.Add(It->GetFName(), Data->GetCurrentValue());
	}
}

void UBaseAttributeSet::PostNetReceive()
{
	Super::PostNetReceive();
	for (TFieldIterator<FStructProperty> It(GetClass()); It; ++It)
	{
		if (It->Struct != FGameplayAttributeData::StaticStruct()) continue;
		const FGameplayAttributeData* Data = It->ContainerPtrToValuePtr<FGameplayAttributeData>(this);
		const float* OldValue = _preNetAttributeValues.Find(It->GetFName());
		if (OldValue && !FMath::IsNearlyEqual(*OldValue, Data->GetCurrentValue()))
			OnAttributeChanged.Broadcast(FGameplayAttribute(*It), *OldValue, Data->GetCurrentValue());
	}
}

void UBaseAttributeSet::PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue)
{
	Super::PostAttributeChange(Attribute, OldValue, NewValue);
	
	// if (GEngine)
	// {
	// 	GEngine->AddOnScreenDebugMessage( -1, 5.f, FColor::Green, FString::Printf(TEXT("Attribute %s changed from %f to %f"), *Attribute.GetName(), OldValue, NewValue));
	// }

	OnAttributeChanged.Broadcast(Attribute, OldValue, NewValue);
}

void UBaseAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	if (Data.EvaluatedData.Attribute != GetHealthAttribute()) return;

	const float Magnitude = Data.EvaluatedData.Magnitude;
	if (Magnitude >= 0.f) return; // damage is negative; ignore heals/regen

	AActor* Victim = GetOwningActor();
	if (!Victim) return;

	AActor* Attacker = Data.EffectSpec.GetContext().GetInstigator();
	if (!Attacker) return;

	USwarmManagerComponent* Swarm = USwarmManagerComponent::Get(Victim);
	if (!Swarm) return;

	Swarm->AddAggro(Victim, Attacker, -Magnitude);
}

void UBaseAttributeSet::OnRep_Health(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UBaseAttributeSet, Health, OldValue);
}

void UBaseAttributeSet::OnRep_MaxHealth(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UBaseAttributeSet, MaxHealth, OldValue);
}

void UBaseAttributeSet::OnRep_Mana(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UBaseAttributeSet, Mana, OldValue);
}

void UBaseAttributeSet::OnRep_MaxMana(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UBaseAttributeSet, MaxMana, OldValue);
}

void UBaseAttributeSet::OnRep_HealthRegenRate(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UBaseAttributeSet, HealthRegenRate, OldValue);
}

void UBaseAttributeSet::OnRep_MovementSpeed(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UBaseAttributeSet, MovementSpeed, OldValue);
}

void UBaseAttributeSet::OnRep_CrouchedSpeed(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UBaseAttributeSet, CrouchedSpeed, OldValue);
}

void UBaseAttributeSet::OnRep_DamageModifierPhysical_Outbound(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UBaseAttributeSet, DamageModifierPhysical_Outbound, OldValue);
}

void UBaseAttributeSet::OnRep_DamageModifierPhysical_Inbound(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UBaseAttributeSet, DamageModifierPhysical_Inbound, OldValue);
}

void UBaseAttributeSet::OnRep_DamageModifierFire_Outbound(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UBaseAttributeSet, DamageModifierFire_Outbound, OldValue);
}

void UBaseAttributeSet::OnRep_DamageModifierFire_Inbound(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UBaseAttributeSet, DamageModifierFire_Inbound, OldValue);
}

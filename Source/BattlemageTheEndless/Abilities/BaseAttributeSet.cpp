// Fill out your copyright notice in the Description page of Project Settings.


#include "BaseAttributeSet.h"
#include "Net/UnrealNetwork.h"

UBaseAttributeSet::UBaseAttributeSet()
{
	Health.SetBaseValue(100.f);
	Health.SetCurrentValue(100.f);

	MaxHealth.SetBaseValue(100.f);
	MaxHealth.SetCurrentValue(100.f);

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
	DOREPLIFETIME_CONDITION_NOTIFY(UBaseAttributeSet, HealthRegenRate, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UBaseAttributeSet, MovementSpeed, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UBaseAttributeSet, CrouchedSpeed, COND_None, REPNOTIFY_Always);
}

void UBaseAttributeSet::PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue)
{
	Super::PostAttributeChange(Attribute, OldValue, NewValue);

	OnAttributeChanged.Broadcast(Attribute, OldValue, NewValue);
}

void UBaseAttributeSet::OnRep_Health(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UBaseAttributeSet, Health, OldValue);
}

void UBaseAttributeSet::OnRep_MaxHealth(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UBaseAttributeSet, MaxHealth, OldValue);
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

#include "EnemyAttributeSet.h"
#include "Net/UnrealNetwork.h"

UEnemyAttributeSet::UEnemyAttributeSet()
{
	Poise.SetBaseValue(100.f);
	Poise.SetCurrentValue(100.f);

	MaxPoise.SetBaseValue(100.f);
	MaxPoise.SetCurrentValue(100.f);

	PoiseRegenRate.SetBaseValue(0.f);
	PoiseRegenRate.SetCurrentValue(0.f);
}

void UEnemyAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(UEnemyAttributeSet, Poise, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UEnemyAttributeSet, MaxPoise, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UEnemyAttributeSet, PoiseRegenRate, COND_None, REPNOTIFY_Always);
}

void UEnemyAttributeSet::OnRep_Poise(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UEnemyAttributeSet, Poise, OldValue);
}

void UEnemyAttributeSet::OnRep_MaxPoise(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UEnemyAttributeSet, MaxPoise, OldValue);
}

void UEnemyAttributeSet::OnRep_PoiseRegenRate(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UEnemyAttributeSet, PoiseRegenRate, OldValue);
}

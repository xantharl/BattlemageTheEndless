#pragma once

#include "CoreMinimal.h"
#include "BaseAttributeSet.h"
#include "EnemyAttributeSet.generated.h"

UCLASS()
class BATTLEMAGETHEENDLESS_API UEnemyAttributeSet : public UBaseAttributeSet
{
	GENERATED_BODY()

public:
	UEnemyAttributeSet();

	UPROPERTY(BlueprintReadOnly, Category = "Poise", ReplicatedUsing = OnRep_Poise)
	FGameplayAttributeData Poise;
	ATTRIBUTE_ACCESSORS(UEnemyAttributeSet, Poise)

	UPROPERTY(BlueprintReadOnly, Category = "Poise", ReplicatedUsing = OnRep_MaxPoise)
	FGameplayAttributeData MaxPoise;
	ATTRIBUTE_ACCESSORS(UEnemyAttributeSet, MaxPoise)

	// Poise recovered per second while not being hit
	UPROPERTY(BlueprintReadOnly, Category = "Poise", ReplicatedUsing = OnRep_PoiseRegenRate)
	FGameplayAttributeData PoiseRegenRate;
	ATTRIBUTE_ACCESSORS(UEnemyAttributeSet, PoiseRegenRate)

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	UFUNCTION()
	virtual void OnRep_Poise(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	virtual void OnRep_MaxPoise(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	virtual void OnRep_PoiseRegenRate(const FGameplayAttributeData& OldValue);
};

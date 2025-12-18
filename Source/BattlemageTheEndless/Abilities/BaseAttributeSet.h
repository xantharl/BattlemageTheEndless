// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "BaseAttributeSet.generated.h"

// Uses macros from AttributeSet.h
#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnAttributeChangedDelegate, const FGameplayAttribute&, Attribute, float, OldValue, float, NewValue);

/**
 * 
 */
UCLASS()
class BATTLEMAGETHEENDLESS_API UBaseAttributeSet : public UAttributeSet
{
	GENERATED_BODY()
	
public:
	UBaseAttributeSet();
	
	UPROPERTY(BlueprintAssignable, Category = "Attributes")
	FOnAttributeChangedDelegate OnAttributeChanged;

	// Current Health, when 0 we expect owner to die unless prevented by an ability. Capped by MaxHealth.
	// Positive changes can directly use this.
	// Negative changes to Health should go through Damage meta attribute.
	UPROPERTY(BlueprintReadOnly, Category = "Health", ReplicatedUsing = OnRep_Health)
	FGameplayAttributeData Health;
	ATTRIBUTE_ACCESSORS(UBaseAttributeSet, Health)

	// MaxHealth is its own attribute since GameplayEffects may modify it
	UPROPERTY(BlueprintReadOnly, Category = "Health", ReplicatedUsing = OnRep_MaxHealth)
	FGameplayAttributeData MaxHealth;
	ATTRIBUTE_ACCESSORS(UBaseAttributeSet, MaxHealth)

	// Health regen rate will passively increase Health every second
	UPROPERTY(BlueprintReadOnly, Category = "Health", ReplicatedUsing = OnRep_HealthRegenRate)
	FGameplayAttributeData HealthRegenRate;
	ATTRIBUTE_ACCESSORS(UBaseAttributeSet, HealthRegenRate)

	UPROPERTY(BlueprintReadOnly, Category = "Movement", ReplicatedUsing = OnRep_MovementSpeed)
	FGameplayAttributeData MovementSpeed;
	ATTRIBUTE_ACCESSORS(UBaseAttributeSet, MovementSpeed)

	UPROPERTY(BlueprintReadOnly, Category = "Movement", ReplicatedUsing = OnRep_CrouchedSpeed)
	FGameplayAttributeData CrouchedSpeed;
	ATTRIBUTE_ACCESSORS(UBaseAttributeSet, CrouchedSpeed)
	
	UPROPERTY(BlueprintReadOnly, Category = "Damage", ReplicatedUsing = OnRep_DamageModifierPhysical_Outbound)
	FGameplayAttributeData DamageModifierPhysical_Outbound;
	ATTRIBUTE_ACCESSORS(UBaseAttributeSet, DamageModifierPhysical_Outbound)
	
	UPROPERTY(BlueprintReadOnly, Category = "Damage", ReplicatedUsing = OnRep_DamageModifierPhysical_Inbound)
	FGameplayAttributeData DamageModifierPhysical_Inbound;
	ATTRIBUTE_ACCESSORS(UBaseAttributeSet, DamageModifierPhysical_Inbound)
	
	UPROPERTY(BlueprintReadOnly, Category = "Damage", ReplicatedUsing = OnRep_DamageModifierFire_Outbound)
	FGameplayAttributeData DamageModifierFire_Outbound;
	ATTRIBUTE_ACCESSORS(UBaseAttributeSet, DamageModifierFire_Outbound)
	
	UPROPERTY(BlueprintReadOnly, Category = "Damage", ReplicatedUsing = OnRep_DamageModifierFire_Inbound)
	FGameplayAttributeData DamageModifierFire_Inbound;
	ATTRIBUTE_ACCESSORS(UBaseAttributeSet, DamageModifierFire_Inbound)
	
	// TODO: Support all elements

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;	

	virtual void PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue) override;

protected:
	/**
	* These OnRep functions exist to make sure that the ability system internal representations are synchronized properly during replication
	**/

	UFUNCTION()
	virtual void OnRep_Health(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	virtual void OnRep_MaxHealth(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	virtual void OnRep_HealthRegenRate(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	virtual void OnRep_MovementSpeed(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	virtual void OnRep_CrouchedSpeed(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	virtual void OnRep_DamageModifierPhysical_Outbound(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	virtual void OnRep_DamageModifierPhysical_Inbound(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	virtual void OnRep_DamageModifierFire_Outbound(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	virtual void OnRep_DamageModifierFire_Inbound(const FGameplayAttributeData& OldValue);
};

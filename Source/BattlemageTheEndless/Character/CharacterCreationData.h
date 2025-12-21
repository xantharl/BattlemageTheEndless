// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "UObject/Object.h"
#include "Kismet/GameplayStatics.h"
#include "CharacterCreationData.generated.h"

class APickupActor;
class ABattlemageTheEndlessCharacter;
/**
 * 
 */
UCLASS(BlueprintType, Blueprintable)
class BATTLEMAGETHEENDLESS_API UCharacterCreationData : public UObject
{
	GENERATED_BODY()
	
public:	
	UPROPERTY(BlueprintReadWrite, Category = "CharacterCreationData", meta = (AllowPrivateAccess = "true"))
	FString CharacterName;
	
	/** Not in use yet **/
	UPROPERTY(BlueprintReadWrite, Category = "CharacterCreationData", meta = (AllowPrivateAccess = "true"))
	FString SelectedClassName;
	
	UPROPERTY(BlueprintReadWrite, Category = "CharacterCreationData", meta = (AllowPrivateAccess = "true"))
	TArray<FString> SelectedArchetypeNames;
	
	UPROPERTY(BlueprintReadWrite, Category = "CharacterCreationData", meta = (AllowPrivateAccess = "true"))
	TArray<FString> SelectedSpellNames;
	
	UFUNCTION(BlueprintCallable, Category = "CharacterCreationData")
	void InitSelections(TArray<FString> ArchetypeNames, TArray<FString> SpellNames);
	
	UPROPERTY(BlueprintReadOnly, Category = "CharacterCreationData", meta = (AllowPrivateAccess = "true"))
	TArray<TSubclassOf<APickupActor>> SelectedPickupActors;
	
	UPROPERTY(BlueprintReadOnly, Category = "CharacterCreationData", meta = (AllowPrivateAccess = "true"))
	TArray<TSubclassOf<UGameplayAbility>> SelectedSpells;
	
private:	
	void BuildSelectedPickupActors(const class UWorld* World);
	void BuildSelectedSpells(class UWorld* World);
};

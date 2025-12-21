// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectComponent.h"
#include "GEComp_DefaultDamage.generated.h"

/**
 * 
 */
UCLASS()
class BATTLEMAGETHEENDLESS_API UGEComp_DefaultDamage : public UGameplayEffectComponent
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Launch", meta = (AllowPrivateAccess = "true"))
	float DefaultValue = 10.f;
};

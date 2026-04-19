#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTDecorator.h"
#include "BTDecorator_TooCloseToActor.generated.h"

UCLASS()
class BATTLEMAGETHEENDLESS_API UBTDecorator_TooCloseToActor : public UBTDecorator
{
	GENERATED_BODY()

public:
	UBTDecorator_TooCloseToActor();

protected:
	UPROPERTY(EditAnywhere, Category = "Condition")
	FBlackboardKeySelector TargetActorKey;

	UPROPERTY(EditAnywhere, Category = "Condition", meta = (ClampMin = "0.0"))
	float MinDistance = 200.f;

	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;
	virtual FString GetStaticDescription() const override;
	virtual void InitializeFromAsset(UBehaviorTree& Asset) override;
};

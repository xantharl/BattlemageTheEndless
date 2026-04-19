#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTDecorator.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Vector.h"
#include "BTDecorator_TooCloseToPoint.generated.h"

UCLASS()
class BATTLEMAGETHEENDLESS_API UBTDecorator_TooCloseToPoint : public UBTDecorator
{
	GENERATED_BODY()

public:
	UBTDecorator_TooCloseToPoint();

protected:
	UPROPERTY(EditAnywhere, Category = "Condition")
	FBlackboardKeySelector TargetPointKey;

	UPROPERTY(EditAnywhere, Category = "Condition", meta = (ClampMin = "0.0"))
	float MinDistance = 200.f;

	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;
	virtual FString GetStaticDescription() const override;
	virtual void InitializeFromAsset(UBehaviorTree& Asset) override;
};

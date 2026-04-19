#include "BTDecorator_TooCloseToPoint.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "AIController.h"

UBTDecorator_TooCloseToPoint::UBTDecorator_TooCloseToPoint()
{
	NodeName = TEXT("Too Close To Point");
	TargetPointKey.AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(UBTDecorator_TooCloseToPoint, TargetPointKey));
}

bool UBTDecorator_TooCloseToPoint::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	const UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!BB)
	{
		return false;
	}

	const AAIController* Controller = OwnerComp.GetAIOwner();
	const APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;
	if (!Pawn)
	{
		return false;
	}

	const FVector TargetPoint = BB->GetValueAsVector(TargetPointKey.SelectedKeyName);
	const float DistanceSq = FVector::DistSquared(Pawn->GetActorLocation(), TargetPoint);
	return DistanceSq < FMath::Square(MinDistance);
}

FString UBTDecorator_TooCloseToPoint::GetStaticDescription() const
{
	return FString::Printf(TEXT("Too Close To Point\nKey: %s\nMin Distance: %.0f"),
		*TargetPointKey.SelectedKeyName.ToString(), MinDistance);
}

void UBTDecorator_TooCloseToPoint::InitializeFromAsset(UBehaviorTree& Asset)
{
	Super::InitializeFromAsset(Asset);
	if (const UBlackboardData* BBAsset = GetBlackboardAsset())
	{
		TargetPointKey.ResolveSelectedKey(*BBAsset);
	}
}

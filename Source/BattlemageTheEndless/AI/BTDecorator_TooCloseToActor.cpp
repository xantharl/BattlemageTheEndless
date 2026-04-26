#include "BTDecorator_TooCloseToActor.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "AIController.h"
#include "VectorTypes.h"

UBTDecorator_TooCloseToActor::UBTDecorator_TooCloseToActor()
{
	NodeName = TEXT("Too Close To Actor");
	TargetActorKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTDecorator_TooCloseToActor, TargetActorKey), AActor::StaticClass());
}

bool UBTDecorator_TooCloseToActor::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
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

	const AActor* TargetActor = Cast<AActor>(BB->GetValueAsObject(TargetActorKey.SelectedKeyName));
	if (!TargetActor)
	{
		return false;
	}
	
	return FMath::Abs(UE::Geometry::Distance(TargetActor->GetActorLocation(), Pawn->GetActorLocation())) < MinDistance;
}

FString UBTDecorator_TooCloseToActor::GetStaticDescription() const
{
	return FString::Printf(TEXT("Too Close To Actor\nKey: %s\nMin Distance: %.0f"),
		*TargetActorKey.SelectedKeyName.ToString(), MinDistance);
}

void UBTDecorator_TooCloseToActor::InitializeFromAsset(UBehaviorTree& Asset)
{
	Super::InitializeFromAsset(Asset);
	if (const UBlackboardData* BBAsset = GetBlackboardAsset())
	{
		TargetActorKey.ResolveSelectedKey(*BBAsset);
	}
}

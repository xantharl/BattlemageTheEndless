// Fill out your copyright notice in the Description page of Project Settings.


#include "WallRunAbility.h"
#include <BattlemageTheEndless/Helpers/VectorMath.h>

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemLog.h"
#include "BattlemageTheEndless/Characters/BattlemageTheEndlessPlayerController.h"
#include "BattlemageTheEndless/Characters/BMageCharacterMovementComponent.h"
#include "GameFramework/PlayerState.h"

UWallRunAbility::UWallRunAbility(const FObjectInitializer& X) : Super(X)
{
	Type = MovementAbilityType::WallRun;
	// Make this ability override sprint
	Priority = 2;
	WallRunCapsule = CreateDefaultSubobject<UCapsuleComponent>(TEXT("WallRunCapsule"));
}

void UWallRunAbility::Init(UCharacterMovementComponent* movement, ACharacter* character, USkeletalMeshComponent* mesh)
{
	// call this first to set shared members
	UMovementAbility::Init(movement, character, mesh);

	// set up the wall run capsule
	// TODO: Figure out why this is null on training dummy
	if (WallRunCapsule)
	{
		WallRunCapsule->InitCapsuleSize(75.f, 75.0f);
		WallRunCapsule->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Overlap);
		WallRunCapsule->AttachToComponent(Character->GetCapsuleComponent(), FAttachmentTransformRules::KeepWorldTransform);
		WallRunCapsule->SetRelativeLocation(FVector(0.f, 0.f, -35.f));
		
		if (Character && Character->IsLocallyControlled())
		{
			WallRunCapsule->OnComponentBeginOverlap.AddDynamic(this, &UWallRunAbility::OnCapsuleBeginOverlap);
			WallRunCapsule->OnComponentEndOverlap.AddDynamic(this, &UWallRunAbility::OnCapsuleEndOverlap);
		}
	}
	
	CharacterBaseGravityScale = Movement->GravityScale;
}

bool UWallRunAbility::ShouldBegin()
{
	if (!WallRunCapsule)
		return false;

	// WallRunHit is set in ObjectIsWallRunnable

	// This is used by the anim graph to determine which way to mirror the wallrun animation
	FVector start = Mesh->GetSocketLocation(FName("feetRaycastSocket"));
	// add 30 degrees to the left of the forward vector to account for wall runs starting near the start of the wall
	FVector end = start + FVector::LeftVector.RotateAngleAxis(WallRunCapsule->GetComponentRotation().Yaw + 30.f, FVector::ZAxisVector) * 200;

	auto params = FCollisionQueryParams(FName(TEXT("LineTrace")), true, Character);
	WallIsToLeft = Traces::LineTraceGeneric(GetWorld(), params, start, end).GetActor() == WallRunObject;

	return true;
}

bool UWallRunAbility::ShouldEnd()
{
	// Raycast from vaultRaycastSocket straight forward to see if Object is in the way
	FVector start = Mesh->GetSocketLocation(FName("feetRaycastSocket"));
	// Cast a ray out in hit normal direction 100 units long
	FVector castVector = (FVector::XAxisVector * 100).RotateAngleAxis(WallRunHit.ImpactNormal.RotateAngleAxis(180.f, FVector::ZAxisVector).Rotation().Yaw, FVector::ZAxisVector);
	FVector end = start + castVector;

	// Perform the raycast
	FHitResult hit;
	FCollisionQueryParams params;
	FCollisionObjectQueryParams objectParams;
	params.AddIgnoredActor(Character);
	GetWorld()->LineTraceSingleByObjectType(hit, start, end, objectParams, params);

	DrawDebugLine(GetWorld(), start, end, FColor::Red, false, 1.0f, 0, 1.0f);

	// If the camera raycast did not hit the object, we are too high to wall run
	return hit.GetActor() == WallRunObject;
}

void UWallRunAbility::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	// end conditions for wall run
	//	no need to check time since there's a timer running for that
	//	if we are on the ground, end the wall run
	//	if a raycast doesn't find the wall, end the wall run
	if (Movement->MovementMode == EMovementMode::MOVE_Walking)// || !ShouldEnd())
	{
		End();
	}
	else if (elapsed > duration_cast<milliseconds>(WallRunGravityDelay * 1000ms)) // if we're still wall running
	{
		// increase gravity linearly based on time elapsed
		Movement->GravityScale += (DeltaTime / (WallRunMaxDuration - WallRunGravityDelay)) * (CharacterBaseGravityScale - WallRunInitialGravityScale);

		// if gravity is over CharacterBaseGravityScale, set it to CharacterBaseGravityScale and end the wall run
		if (Movement->GravityScale > CharacterBaseGravityScale)
		{
			Movement->GravityScale = CharacterBaseGravityScale;
			End();
		}
	}
}
void UWallRunAbility::Begin(const FMovementEventData& MovementEventData)
{	
	// the super should always be called first as it sets isactive
	Super::Begin(MovementEventData);
	
	//CharacterBaseGravityScale = MovementEventData.OptionalFloat != 0.f ? MovementEventData.OptionalFloat : Movement->GravityScale;
	if (MovementEventData.OptionalHitResult.IsValidBlockingHit())
	{
		WallRunHit = MovementEventData.OptionalHitResult;
	}
	
	Movement->GravityScale = WallRunInitialGravityScale;
	// kill any vertical movement
	Movement->Velocity.Z = 0.f;	

	// this is to work around double jump logic in ACharacter which auto increments the jump count if we're falling
	Character->JumpCurrentCount -= 2;

	// get vectors parallel to the wall
	const FRotator PossibleWallRunDirectionOne = WallRunHit.ImpactNormal.RotateAngleAxis(90.f, FVector::ZAxisVector).Rotation();
	const FRotator PossibleWallRunDirectionTwo = WallRunHit.ImpactNormal.RotateAngleAxis(-90.f, FVector::ZAxisVector).Rotation();
	const float LookDirection = Movement->GetLastUpdateRotation().Yaw;
	// find the closest direction to the character's current look direction
	const auto DeltaOne = FMath::FindDeltaAngleDegrees(LookDirection, PossibleWallRunDirectionOne.Yaw);
	const auto DeltaTwo = FMath::FindDeltaAngleDegrees(LookDirection, PossibleWallRunDirectionTwo.Yaw);
	const FRotator BestOption = FMath::Abs(DeltaOne) < FMath::Abs(DeltaTwo) ? PossibleWallRunDirectionOne : PossibleWallRunDirectionTwo;
	
	// consumed by caller in character
	TargetRotation = FRotator(0.f, BestOption.Yaw, 0.f);

	// redirect character's velocity to be parallel to the wall, ignore input
	Movement->Velocity = TargetRotation.Vector() * Movement->Velocity.Size();

	Character->bUseControllerRotationYaw = false;
	const auto State = Character->GetPlayerState();
	if (!State)
	{
		UE_LOG(LogTemp, Warning, TEXT("WallRunAbility: Character has no PlayerState, cannot set wall run camera settings"));
	}
	// set max pan angle to 60 degrees
	else if (APlayerCameraManager* cameraManager = UGameplayStatics::GetPlayerCameraManager(GetWorld(), State->GetPlayerId()))
	{
		float currentYaw = TargetRotation.Yaw;
		cameraManager->ViewYawMax = currentYaw + 90.f;
		cameraManager->ViewYawMin = currentYaw - 90.f;
	}

	// set air control to 100% to allow for continued movement parallel to the wall
	Movement->AirControl = 1.0f;

	// Wall running refunds some jump charges
	if (Character->JumpCurrentCount > 0)
	{
		Character->JumpCurrentCount = FMath::Max(0, Character->JumpCurrentCount - WallRunJumpRefundCount);
	}

	// set a timer to end the wall run
	Character->GetWorldTimerManager().SetTimer(WallRunTimer, this, &UWallRunAbility::OnEndTimer, WallRunMaxDuration, false);
}
void UWallRunAbility::End(bool bForce)
{
	Character->bUseControllerRotationYaw = true;

	if (CharacterBaseGravityScale < 0.001f && GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("WallRunAbility: CharacterBaseGravityScale is zero, resetting to 1.0f"));
		CharacterBaseGravityScale = 1.0f;
	}
	Movement->GravityScale = CharacterBaseGravityScale;
	WallRunObject = nullptr;

	// reset to full rotation
	if (APlayerCameraManager* cameraManager = UGameplayStatics::GetPlayerCameraManager(GetWorld(), 0))
	{
		cameraManager->ViewYawMax = 359.998993f;
		cameraManager->ViewYawMin = 0.f;
	}

	Super::End(bForce);
}

bool UWallRunAbility::ObjectIsWallRunnable(AActor* actor, USkeletalMeshComponent* mesh)
{
	// if the object is a pawn, was cannot wallrun on it
	if (actor->IsA(APawn::StaticClass()))
		return false;

	bool drawTrace = true;

	// Repeat the same process but use socket feetRaycastSocket
	FHitResult hit = Traces::LineTraceMovementVector(Character, Movement, mesh, FName("feetRaycastSocket"), 500, drawTrace);

	bool hitLeft = false;
	bool hitRight = false;
	// If we didn't hit the object, try again with a vector 45 degress left
	if (hit.GetActor() != actor)
	{
		hit = Traces::LineTraceMovementVector(Character, Movement, mesh, FName("feetRaycastSocket"), 500, drawTrace, FColor::Blue, -45.f);

		hitLeft = hit.GetActor() == actor;
		// if we still didn't hit, try 45 right
		if (!hitLeft)
		{
			hit = Traces::LineTraceMovementVector(Character, Movement, mesh, FName("feetRaycastSocket"), 500, drawTrace, FColor::Emerald, 45.f);
			hitRight = hit.GetActor() == actor;
		}

		// We have now tried all supported directions, if we didn't hit the object, we can't wall run
		if (!hitLeft && !hitRight)
			return false;
	}

	FVector impactDirection = hit.ImpactNormal.RotateAngleAxis(180.f, FVector::ZAxisVector);
	float yawDifference = VectorMath::Vector2DRotationDifference(Movement->Velocity, impactDirection);

	// there is no need to correct for the left or right hit cases since the impact normal will be the same

	//if (GEngine)
	//	GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Green, FString::Printf(TEXT("'%f' yaw diff"), yawDifference));

	// wall run if we're more than 20 degrees rotated from the wall but less than 
	if (yawDifference > 20.f && yawDifference < 180.f)
	{
		/*if (GEngine)
			GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Green, TEXT("Starting Wall Run"));*/
		WallRunHit = hit;
		return true;
	}
	return false;
}

void UWallRunAbility::OnCapsuleBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{	
	// We only want local clients to trigger wall runs and request them on the server
	if (!Character->IsLocallyControlled()) // TODO: Account for AI?
		return;
	
	if(Movement->MovementMode != MOVE_Falling)
		return;
		
	// Handle through ASC
	if (auto ASC = Character->GetComponentByClass<UAbilitySystemComponent>())
	{
		auto WallRunTag = FGameplayTag::RequestGameplayTag("Movement.WallRun");
		WallRunObject = OtherActor;
		if (!ObjectIsWallRunnable(WallRunObject, Mesh) || !ShouldBegin())
			return;
				
		FGameplayEventData EventData;
		EventData.Instigator = Character;
		EventData.Target = Character;
		EventData.EventTag = WallRunTag;
		
		// This needs to happen before we call the local since we sometimes modify dependent character data in begin ability
		auto MovementEventData = BuildMovementEventData();
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Character, EventData.EventTag,EventData);

		if (auto CastedController = Cast<ABattlemageTheEndlessPlayerController>(Character->GetController()))
		{
			// tell server about the overlapped actor
			CastedController->Server_HandleMovementEvent(WallRunTag, MovementEventData);
		}
	}
}

// TODO: Notify the character that the wall run has ended
void UWallRunAbility::OnCapsuleEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	// This is a bit of a hack but we immediately fire EndOverlap from server due to replication lag
	if (elapsed < duration_cast<milliseconds>(250ms))
		return;
	
 	if (IsGAActive() && WallRunObject == OtherActor)
		End();
}

FMovementEventData UWallRunAbility::BuildMovementEventData() const
{
	auto PawnMovement = Character->GetMovementComponent();
	if (!PawnMovement)
	{
		UE_LOG(LogTemp, Warning, TEXT( "UWallRunAbility::BuildMovementEventDataFromPawn - Pawn %s has no "
						   "MovementComponent, cannot build MovementEventData" ), *Character->GetName() );
		return FMovementEventData();
	}
	
	FMovementEventData ReturnData = FMovementEventData();
	ReturnData.OptionalHitResult = WallRunHit;
	ReturnData.OptionalFloat = Movement->GravityScale;
	return ReturnData;
}
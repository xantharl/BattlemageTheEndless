// Fill out your copyright notice in the Description page of Project Settings.

#include "DodgeAbility.h"
#include "BattlemageTheEndless/Characters/BMageCharacterMovementComponent.h"

UDodgeAbility::UDodgeAbility(const FObjectInitializer& X) : Super(X)
{
	Type = MovementAbilityType::Dodge;
	// Make this ability override sprint
	Priority = 2;
}

void UDodgeAbility::Begin(const FMovementEventData& MovementEventData)
{
	FVector InputVector;
	// If MovementEventData was not provided, we are using the local path and can rely on local vars
	LastInputVector = MovementEventData.OptionalVector == FVector::ZeroVector ? Character->GetLastMovementInputVector() : MovementEventData.OptionalVector;
	PreviousFriction = MovementEventData.OptionalFloat == 0.f ? Movement->GroundFriction : MovementEventData.OptionalFloat;
	if (LastInputVector != FVector::ZeroVector)
	{
		InputVector = LastInputVector;
	}
	else
	{
		InputVector = Character->GetLastMovementInputVector();
	}
	// account for camera rotation: rotate world input into pawn-local space (X=forward, Y=right)
	InputVector = InputVector.RotateAngleAxis(Movement->GetLastUpdateRotation().GetInverse().Yaw, FVector::ZAxisVector);

	FVector DodgeVector = ComputeDodgeImpulse(InputVector);

	// Launch the character
	Movement->GroundFriction = 0.0f;
	FVector WorldImpulse = Character->GetActorTransform().TransformVectorNoScale(DodgeVector);

	// modify impulse if in air
	if (!Movement->IsWalking())
	{
		float LookPitch = FRotator::NormalizeAxis(Character->GetControlRotation().Pitch);
		//Character->bUseControllerRotationPitch = true;
		WorldImpulse = WorldImpulse.RotateAngleAxis(-LookPitch, Character->GetActorRightVector());
		WorldImpulse *= ImpulseMultiplierAir;

		// Fly for the dodge duration so gravity doesn't drag it down; restored in End().
		// Set velocity directly rather than LaunchCharacter, whose pending launch forces MOVE_Falling.
		PreviousMovementMode = Movement->MovementMode;
		bChangedMovementMode = true;
		Movement->Velocity = WorldImpulse;
		Movement->SetMovementMode(MOVE_Flying);

		// The dodge montage's root motion is horizontal-only and would otherwise flatten the
		// trajectory; have the movement component rotate it to follow look pitch.
		if (UBMageCharacterMovementComponent* MageMovement = Cast<UBMageCharacterMovementComponent>(Movement))
		{
			MageMovement->bAirDodgeActive = true;
			MageMovement->AirDodgePitch = LookPitch;
			UE_LOG(LogTemp, Warning, TEXT("[AirDodge] Begin: LookPitch=%.1f WorldImpulse=%s"), LookPitch, *WorldImpulse.ToString());
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[AirDodge] Begin: cast to UBMageCharacterMovementComponent FAILED"));
		}
	}
	else
	{
		Character->LaunchCharacter(WorldImpulse, true, true);
	}

	Character->GetWorldTimerManager().SetTimer(DodgeEndTimer, this, &UDodgeAbility::OnEndTimer, DodgeDurationSeconds, false);

	Super::Begin(MovementEventData);
}

void UDodgeAbility::End(bool bForce)
{
	Super::End(bForce);
	Movement->GroundFriction = PreviousFriction;
	if (bChangedMovementMode)
	{
		// FRotator Rotation = Character->GetActorRotation();
		// Rotation.Pitch = 0.f;
		// Character->SetActorRotation(Rotation);
		// Character->bUseControllerRotationPitch = false;
		Movement->SetMovementMode(MOVE_Falling);
		bChangedMovementMode = false;
		if (UBMageCharacterMovementComponent* MageMovement = Cast<UBMageCharacterMovementComponent>(Movement))
			MageMovement->bAirDodgeActive = false;
	}
}

FVector UDodgeAbility::ComputeDodgeImpulse(const FVector& LocalInput) const
{
	FVector2D Dir(LocalInput.X, LocalInput.Y);
	if (Dir.IsNearlyZero())
	{
		return DodgeImpulseForward;
	}
	Dir.Normalize();

	// DodgeImpulseLateral is configured for the "left" side (negative Y). Mirror Y for the right-side cardinal.
	const FVector LeftImpulse = DodgeImpulseLateral;
	const FVector RightImpulse(DodgeImpulseLateral.X, -DodgeImpulseLateral.Y, DodgeImpulseLateral.Z);

	// Angle: 0 = forward, +pi/2 = right, +/-pi = back, -pi/2 = left
	const float Angle = FMath::Atan2(Dir.Y, Dir.X);

	FVector A, B;
	float Alpha;
	if (Angle >= 0.f && Angle <= HALF_PI)            // forward -> right
	{
		A = DodgeImpulseForward;  B = RightImpulse;        Alpha = Angle / HALF_PI;
	}
	else if (Angle > HALF_PI)                         // right -> back
	{
		A = RightImpulse;         B = DodgeImpulseBackward; Alpha = (Angle - HALF_PI) / HALF_PI;
	}
	else if (Angle >= -HALF_PI)                       // forward -> left
	{
		A = DodgeImpulseForward;  B = LeftImpulse;         Alpha = -Angle / HALF_PI;
	}
	else                                              // left -> back
	{
		A = LeftImpulse;          B = DodgeImpulseBackward; Alpha = (-Angle - HALF_PI) / HALF_PI;
	}

	return FMath::Lerp(A, B, Alpha);
}

FMovementEventData UDodgeAbility::BuildMovementEventData() const
{
	auto PawnMovement = Character->GetMovementComponent();
	if (!PawnMovement)
	{
		UE_LOG(LogTemp, Warning, TEXT( "UMovementAbility::BuildMovementEventDataFromPawn - Pawn %s has no "
						   "MovementComponent, cannot build MovementEventData" ), *Character->GetName() );
		return FMovementEventData();
	}
	
	FMovementEventData ReturnData = FMovementEventData();
	ReturnData.OptionalVector = PawnMovement->GetLastInputVector();						
	if (IsValid(Movement))
		ReturnData.OptionalFloat = Movement->GroundFriction;
	else
		UE_LOG(LogTemp, Warning, TEXT( "UMovementAbility::BuildMovementEventDataFromPawn - Character %s has no "
						   "CharacterMovementComponent, cannot populate MovementEventData.OptionalFloat" ), *GetName() );
	return ReturnData;
}

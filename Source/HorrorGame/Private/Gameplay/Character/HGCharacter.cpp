// Fill out your copyright notice in the Description page of Project Settings.


#include "HorrorGame/Public/Gameplay/Character/HGCharacter.h"

#include "EnhancedInputComponent.h"
#include "DefaultMovementSet/CharacterMoverComponent.h"
#include "MoveLibrary/BasedMovementUtils.h"


AHGCharacter::AHGCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	
	MotionComponent = CreateDefaultSubobject<UCharacterMoverComponent>(TEXT("MoverComponent"));
	
	SetReplicatingMovement(false);
}

void AHGCharacter::BeginPlay()
{
	Super::BeginPlay();
	
	PlayerController = Cast<APlayerController>(GetController());
}

void AHGCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	if (PlayerController)
	{
		PlayerController->AddYawInput(CachedLookInput.Yaw * LookRateYaw * DeltaTime);
		PlayerController->AddPitchInput(-CachedLookInput.Pitch * LookRatePitch * DeltaTime);
	}

	CachedLookInput = FRotator::ZeroRotator;
}

void AHGCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	
	if (!IsValid(PlayerInputs)) return;
	
	if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		EIC->BindAction(PlayerInputs->Inputs[EPlayerInputs::Look], ETriggerEvent::Triggered, this, &AHGCharacter::OnLookTriggered);
		EIC->BindAction(PlayerInputs->Inputs[EPlayerInputs::Look], ETriggerEvent::Completed, this, &AHGCharacter::OnLookCompleted);
		
		EIC->BindAction(PlayerInputs->Inputs[EPlayerInputs::Move], ETriggerEvent::Triggered, this, &AHGCharacter::OnMoveTriggered);
		EIC->BindAction(PlayerInputs->Inputs[EPlayerInputs::Move], ETriggerEvent::Completed, this, &AHGCharacter::OnMoveCompleted);
	}
}

void AHGCharacter::OnMoveTriggered(const FInputActionValue& Value)
{
	const FVector MovementVector = Value.Get<FVector>();
	CachedMoveInputIntent.X = FMath::Clamp(MovementVector.X, -1.0f, 1.0f);
	CachedMoveInputIntent.Y = FMath::Clamp(MovementVector.Y, -1.0f, 1.0f);
	CachedMoveInputIntent.Z = FMath::Clamp(MovementVector.Z, -1.0f, 1.0f);
}

void AHGCharacter::OnMoveCompleted(const FInputActionValue& Value)
{
	CachedMoveInputIntent = FVector::ZeroVector;
}

void AHGCharacter::OnLookTriggered(const FInputActionValue& Value)
{
	const FVector2D LookVector = Value.Get<FVector2D>();
	CachedLookInput.Yaw = CachedTurnInput.Yaw = FMath::Clamp(LookVector.X, -1.0f, 1.0f);
	CachedLookInput.Pitch = CachedTurnInput.Pitch = FMath::Clamp(LookVector.Y, -1.0f, 1.0f);
}

void AHGCharacter::OnLookCompleted(const FInputActionValue& Value)
{
	CachedLookInput = FRotator::ZeroRotator;
	CachedTurnInput = FRotator::ZeroRotator;
}

void AHGCharacter::ProduceInput_Implementation(int32 SimTimeMs, FMoverInputCmdContext& InputCmdResult)
{
	IMoverInputProducerInterface::ProduceInput_Implementation(SimTimeMs, InputCmdResult);
	
	FCharacterDefaultInputs& CharacterInputs = InputCmdResult.InputCollection.FindOrAddMutableDataByType<FCharacterDefaultInputs>();

	if (GetController() == nullptr)
	{
		if (GetLocalRole() == ENetRole::ROLE_Authority && GetRemoteRole() == ENetRole::ROLE_SimulatedProxy)
		{
			static const FCharacterDefaultInputs DoNothingInput;
			CharacterInputs = DoNothingInput;
		}

		return;
	}
	
	CharacterInputs.ControlRotation = FRotator::ZeroRotator;

	APlayerController* PC = Cast<APlayerController>(GetController());
	if (PC)
	{
		CharacterInputs.ControlRotation = PC->GetControlRotation();
	}

	//bool bRequestedNavMovement = false;
	//if (NavMoverComponent)
	//{
	//	bRequestedNavMovement = NavMoverComponent->ConsumeNavMovementData(CachedMoveInputIntent, CachedMoveInputVelocity);
	//}
	
	// Favor velocity input 
	bool bUsingInputIntentForMove = CachedMoveInputVelocity.IsZero();

	if (bUsingInputIntentForMove)
	{
		const FVector FinalDirectionalIntent = CharacterInputs.ControlRotation.RotateVector(CachedMoveInputIntent);
		CharacterInputs.SetMoveInput(EMoveInputType::DirectionalIntent, FinalDirectionalIntent);
	}
	else
	{
		CharacterInputs.SetMoveInput(EMoveInputType::Velocity, CachedMoveInputVelocity);
	}

	// Normally cached input is cleared by OnMoveCompleted input event but that won't be called if movement came from nav movement
	//if (bRequestedNavMovement)
	//{
	//	CachedMoveInputIntent = FVector::ZeroVector;
	//	CachedMoveInputVelocity = FVector::ZeroVector;
	//}
	
	static float RotationMagMin(1e-3);

	const bool bHasAffirmativeMoveInput = (CharacterInputs.GetMoveInput().Size() >= RotationMagMin);
	
	// Figure out intended orientation
	CharacterInputs.OrientationIntent = FVector::ZeroVector;


	if (bHasAffirmativeMoveInput)
	{
		if (bOrientRotationToMovement)
		{
			// set the intent to the actors movement direction
			CharacterInputs.OrientationIntent = CharacterInputs.GetMoveInput().GetSafeNormal();
		}
		else
		{
			// set intent to the control rotation - often a player's camera rotation
			CharacterInputs.OrientationIntent = CharacterInputs.ControlRotation.Vector().GetSafeNormal();
		}

		LastAffirmativeMoveInput = CharacterInputs.GetMoveInput();
	}
	else if (bMaintainLastInputOrientation)
	{
		// There is no movement intent, so use the last-known affirmative move input
		CharacterInputs.OrientationIntent = LastAffirmativeMoveInput;
	}
	
	CharacterInputs.bIsJumpPressed = bIsJumpPressed;
	CharacterInputs.bIsJumpJustPressed = bIsJumpJustPressed;

	if (bShouldToggleFlying)
	{
		if (!bIsFlyingActive)
		{
			CharacterInputs.SuggestedMovementMode = DefaultModeNames::Flying;
		}
		else
		{
			CharacterInputs.SuggestedMovementMode = DefaultModeNames::Falling;
		}

		bIsFlyingActive = !bIsFlyingActive;
	}
	else
	{
		CharacterInputs.SuggestedMovementMode = NAME_None;
	}

	// Convert inputs to be relative to the current movement base (depending on options and state)
	CharacterInputs.bUsingMovementBase = false;

	if (bUseBaseRelativeMovement)
	{
		if (const UCharacterMoverComponent* MoverComp = GetComponentByClass<UCharacterMoverComponent>())
		{
			if (UPrimitiveComponent* MovementBase = MoverComp->GetMovementBase())
			{
				FName MovementBaseBoneName = MoverComp->GetMovementBaseBoneName();

				FVector RelativeMoveInput, RelativeOrientDir;

				UBasedMovementUtils::TransformWorldDirectionToBased(MovementBase, MovementBaseBoneName, CharacterInputs.GetMoveInput(), RelativeMoveInput);
				UBasedMovementUtils::TransformWorldDirectionToBased(MovementBase, MovementBaseBoneName, CharacterInputs.OrientationIntent, RelativeOrientDir);

				CharacterInputs.SetMoveInput(CharacterInputs.GetMoveInputType(), RelativeMoveInput);
				CharacterInputs.OrientationIntent = RelativeOrientDir;

				CharacterInputs.bUsingMovementBase = true;
				CharacterInputs.MovementBase = MovementBase;
				CharacterInputs.MovementBaseBoneName = MovementBaseBoneName;
			}
		}
	}

	// Clear/consume temporal movement inputs. We are not consuming others in the event that the game world is ticking at a lower rate than the Mover simulation. 
	// In that case, we want most input to carry over between simulation frames.
	{

		bIsJumpJustPressed = false;
		bShouldToggleFlying = false;
	}
}


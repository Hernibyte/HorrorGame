// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MoverSimulationTypes.h"
#include "GameFramework/Pawn.h"
#include "HorrorGame/Public/DataAssets/Inputs/HGPlayerInputsDataAsset.h"
#include "HGCharacter.generated.h"

class UCharacterMoverComponent;

UCLASS()
class HORRORGAME_API AHGCharacter : public APawn, public IMoverInputProducerInterface
{
	GENERATED_BODY()

public:
	AHGCharacter();
	
	virtual void BeginPlay() override;

	virtual void Tick(float DeltaTime) override;
	
	virtual void ProduceInput_Implementation(int32 SimTimeMs, FMoverInputCmdContext& InputCmdResult) override;
	
	// Whether or not we author our movement inputs relative to whatever base we're standing on, or leave them in world space. Only applies if standing on a base of some sort.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=MoverExamples)
	bool bUseBaseRelativeMovement = true;
	
	/**
	 * If true, rotate the Character toward the direction the actor is moving
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=MoverExamples)
	bool bOrientRotationToMovement = true;
	
	/**
	 * If true, the actor will continue orienting towards the last intended orientation (from input) even after movement intent input has ceased.
	 * This makes the character finish orienting after a quick stick flick from the player.  If false, character will not turn without input.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = MoverExamples)
	bool bMaintainLastInputOrientation = false;

protected:
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	
	UFUNCTION()
	void OnMoveTriggered(const FInputActionValue& Value);
	void OnMoveCompleted(const FInputActionValue& Value);
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UHGPlayerInputsDataAsset> PlayerInputs;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float LookRateYaw = 100.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float LookRatePitch = 100.f;
	
private:
	
	void OnLookTriggered(const FInputActionValue& Value);
	void OnLookCompleted(const FInputActionValue& Value);
	
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UCharacterMoverComponent> MotionComponent;
	
	FVector LastAffirmativeMoveInput = FVector::ZeroVector;	// Movement input (intent or velocity) the last time we had one that wasn't zero

	FVector CachedMoveInputIntent = FVector::ZeroVector;
	FVector CachedMoveInputVelocity = FVector::ZeroVector;
	
	FRotator CachedTurnInput = FRotator::ZeroRotator;
	FRotator CachedLookInput = FRotator::ZeroRotator;
	
	UPROPERTY()
	TObjectPtr<APlayerController> PlayerController;
	
	bool bIsJumpJustPressed = false;
	bool bIsJumpPressed = false;
	bool bIsFlyingActive = false;
	bool bShouldToggleFlying = false;
};

// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "InputAction.h"
#include "Engine/DataAsset.h"
#include "HorrorGame/Public/HGTypes.h"
#include "HGPlayerInputsDataAsset.generated.h"

/**
 * 
 */
UCLASS()
class HORRORGAME_API UHGPlayerInputsDataAsset : public UDataAsset
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere)
	TMap<EPlayerInputs, UInputAction*> Inputs;
	
};

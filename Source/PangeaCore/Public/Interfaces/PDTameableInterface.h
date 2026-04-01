// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Types/TamingTypes.h"
#include "UObject/Interface.h"
#include "PDTameableInterface.generated.h"

// This class does not need to be modified.
UINTERFACE()
class UPDTameableInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class PANGEACORE_API IPDTameableInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	virtual ETameState GetTameState() const = 0;
	virtual ETamedRole GetTamedRole() const = 0;
	virtual bool CanBeTamed() const = 0;
	virtual void StartTameAttempt(AActor* Instigator) = 0;
	virtual void HandleLoadedActor() = 0;
	
};

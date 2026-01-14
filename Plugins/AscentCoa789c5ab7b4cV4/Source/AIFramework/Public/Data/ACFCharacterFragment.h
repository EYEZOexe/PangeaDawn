// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2025. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "ACFCharacterFragment.generated.h"


/*
 * Base class for pawn fragments used to compose pawn definitions in ACF
 * Can be extended in Blueprint to add custom data to characters.
 */
UCLASS(Blueprintable, EditInlineNew, DefaultToInstanced)
class AIFRAMEWORK_API UACFCharacterFragment : public UObject
{
	GENERATED_BODY()

public:

	UACFCharacterFragment() {};

public:

	UFUNCTION(BlueprintNativeEvent, Category = ACF)
	void ApplyFragment(APawn* pawnOwner);

};

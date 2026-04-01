#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "PangeaDefinitionFragment.generated.h"

class UPangeaCreatureDefinition;

UCLASS(Abstract, BlueprintType, Blueprintable, EditInlineNew, DefaultToInstanced)
class PANGEACORE_API UPangeaDefinitionFragment : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category="Definition")
	UPangeaCreatureDefinition* GetOwningDefinition() const;
};

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "BaseUpgradeFragment.generated.h"

class UBaseUpgradeDefinition;

UCLASS(Abstract, BlueprintType, Blueprintable, EditInlineNew, DefaultToInstanced)
class PANGEABASEUPGRADESYSTEM_API UBaseUpgradeFragment : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category="Upgrade")
	UBaseUpgradeDefinition* GetOwningDefinition() const;
};

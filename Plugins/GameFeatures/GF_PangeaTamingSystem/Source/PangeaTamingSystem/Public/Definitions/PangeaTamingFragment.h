#pragma once

#include "CoreMinimal.h"
#include "Definitions/PangeaDefinitionFragment.h"
#include "PangeaTamingFragment.generated.h"

class UTameSpeciesConfig;

UCLASS(BlueprintType, EditInlineNew)
class PANGEATAMINGSYSTEM_API UPangeaTamingFragment : public UPangeaDefinitionFragment
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Taming")
	TObjectPtr<UTameSpeciesConfig> TameSpeciesConfig;
};

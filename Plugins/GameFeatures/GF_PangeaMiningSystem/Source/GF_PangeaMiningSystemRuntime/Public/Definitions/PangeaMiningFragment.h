#pragma once

#include "CoreMinimal.h"
#include "Definitions/PangeaDefinitionFragment.h"
#include "PangeaMiningFragment.generated.h"

class UMiningSiteDefinition;

UCLASS(BlueprintType, EditInlineNew)
class GF_PANGEAMININGSYSTEMRUNTIME_API UPangeaMiningFragment : public UPangeaDefinitionFragment
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Mining")
	TObjectPtr<UMiningSiteDefinition> MiningSiteDefinition;
};

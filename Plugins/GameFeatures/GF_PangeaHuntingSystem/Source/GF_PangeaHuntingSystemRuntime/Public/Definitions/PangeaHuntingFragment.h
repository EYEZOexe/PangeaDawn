#pragma once

#include "CoreMinimal.h"
#include "Definitions/PangeaDefinitionFragment.h"
#include "PangeaHuntingFragment.generated.h"

class UHuntSpeciesConfig;

UCLASS(BlueprintType, EditInlineNew)
class GF_PANGEAHUNTINGSYSTEMRUNTIME_API UPangeaHuntingFragment : public UPangeaDefinitionFragment
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Hunting")
	TObjectPtr<UHuntSpeciesConfig> HuntSpeciesConfig;
};

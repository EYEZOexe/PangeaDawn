#pragma once

#include "CoreMinimal.h"
#include "Definitions/BaseUpgradeFragment.h"
#include "BaseSaveRulesFragment.generated.h"

UCLASS(BlueprintType)
class PANGEABASEUPGRADESYSTEM_API UBaseSaveRulesFragment : public UBaseUpgradeFragment
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Save")
	bool bReplayCompletedMilestonesOnLoad = true;
};

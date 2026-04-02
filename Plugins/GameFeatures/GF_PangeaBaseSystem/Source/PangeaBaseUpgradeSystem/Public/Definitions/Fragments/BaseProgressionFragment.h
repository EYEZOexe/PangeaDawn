#pragma once

#include "CoreMinimal.h"
#include "Definitions/BaseUpgradeFragment.h"
#include "Types/BaseUpgradeTypes.h"
#include "BaseProgressionFragment.generated.h"

UCLASS(BlueprintType)
class PANGEABASEUPGRADESYSTEM_API UBaseProgressionFragment : public UBaseUpgradeFragment
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Upgrade")
	TArray<FUpgradeLevelDefinition> Levels;
};

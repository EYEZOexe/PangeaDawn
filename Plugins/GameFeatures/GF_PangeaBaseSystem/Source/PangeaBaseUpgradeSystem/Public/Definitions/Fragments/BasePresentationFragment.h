#pragma once

#include "CoreMinimal.h"
#include "Definitions/BaseUpgradeFragment.h"
#include "BasePresentationFragment.generated.h"

UCLASS(BlueprintType)
class PANGEABASEUPGRADESYSTEM_API UBasePresentationFragment : public UBaseUpgradeFragment
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presentation")
	FText InteractionText;
};

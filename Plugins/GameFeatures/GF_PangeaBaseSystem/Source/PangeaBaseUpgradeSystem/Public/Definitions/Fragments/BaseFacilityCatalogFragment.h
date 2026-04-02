#pragma once

#include "CoreMinimal.h"
#include "Definitions/BaseUpgradeFragment.h"
#include "Types/BaseUpgradeTypes.h"
#include "BaseFacilityCatalogFragment.generated.h"

UCLASS(BlueprintType)
class PANGEABASEUPGRADESYSTEM_API UBaseFacilityCatalogFragment : public UBaseUpgradeFragment
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Facility")
	TArray<FFacilityGroupReference> FacilityGroups;
};

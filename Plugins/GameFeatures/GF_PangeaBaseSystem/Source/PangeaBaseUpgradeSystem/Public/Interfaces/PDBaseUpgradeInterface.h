#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UObject/Interface.h"
#include "Types/BaseUpgradeTypes.h"
#include "PDBaseUpgradeInterface.generated.h"

class UUpgradeRequirement;

UINTERFACE(BlueprintType)
class PANGEABASEUPGRADESYSTEM_API UPDBaseUpgradeInterface : public UInterface
{
	GENERATED_BODY()
};

class PANGEABASEUPGRADESYSTEM_API IPDBaseUpgradeInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Upgrade")
	bool LoadCompletedMilestonesForContext(UObject* PlayerContext);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Upgrade")
	bool CanUpgradeToNextLevelForContext(UObject* PlayerContext) const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Upgrade")
	bool TryUpgradeToNextLevel(UObject* PlayerContext);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Upgrade")
	int32 GetCurrentUpgradeLevel() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Upgrade")
	bool IsUpgradeMilestoneCompleted(FGameplayTag MilestoneTag) const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Upgrade|UI")
	bool GetNextUpgradeLevelDefinition(FUpgradeLevelDefinition& OutLevel) const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Upgrade|UI")
	void GetUnmetRequirementsForNextUpgrade(UObject* PlayerContext, TArray<UUpgradeRequirement*>& OutRequirements) const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Upgrade|UI")
	void GetFacilitiesUnlockedAtUpgradeLevel(int32 Level, TArray<FGameplayTag>& OutFacilities) const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Upgrade|UI")
	FText GetUpgradeFacilityDisplayName(FGameplayTag FacilityTag) const;
};

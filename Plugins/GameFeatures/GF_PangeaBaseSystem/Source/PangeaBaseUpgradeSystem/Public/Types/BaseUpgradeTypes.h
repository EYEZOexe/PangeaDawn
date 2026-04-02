#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "BaseUpgradeTypes.generated.h"

class UUpgradeAction;
class UUpgradeRequirement;

USTRUCT(BlueprintType)
struct PANGEABASEUPGRADESYSTEM_API FFacilityGroupReference
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Facility")
	FText FacilityGroupName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Facility")
	FGameplayTag FacilityTag;
};

USTRUCT(BlueprintType)
struct PANGEABASEUPGRADESYSTEM_API FUpgradeMilestoneDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Upgrade")
	FGameplayTag MilestoneTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Upgrade")
	FText DisplayName;

	UPROPERTY(EditAnywhere, Instanced, BlueprintReadOnly, Category="Upgrade")
	TArray<TObjectPtr<UUpgradeRequirement>> Requirements;

	UPROPERTY(EditAnywhere, Instanced, BlueprintReadOnly, Category="Upgrade")
	TArray<TObjectPtr<UUpgradeAction>> Actions;
};

USTRUCT(BlueprintType)
struct PANGEABASEUPGRADESYSTEM_API FUpgradeLevelDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Upgrade")
	int32 Level = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Upgrade")
	TArray<FUpgradeMilestoneDefinition> Milestones;
};

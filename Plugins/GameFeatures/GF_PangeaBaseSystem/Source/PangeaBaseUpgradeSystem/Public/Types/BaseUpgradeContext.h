#pragma once

#include "CoreMinimal.h"
#include "BaseUpgradeContext.generated.h"

class APawn;
class APlayerController;
class AVillageBase;
class UPangeaFacilityManagerComponent;
class UPangeaUpgradeSystemComponent;

USTRUCT(BlueprintType)
struct PANGEABASEUPGRADESYSTEM_API FBaseUpgradeContext
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category="Upgrade")
	TObjectPtr<UObject> SourceObject = nullptr;

	UPROPERTY(BlueprintReadOnly, Category="Upgrade")
	TObjectPtr<AVillageBase> Village = nullptr;

	UPROPERTY(BlueprintReadOnly, Category="Upgrade")
	TObjectPtr<APawn> InteractingPawn = nullptr;

	UPROPERTY(BlueprintReadOnly, Category="Upgrade")
	TObjectPtr<APlayerController> PlayerController = nullptr;

	UPROPERTY(BlueprintReadOnly, Category="Upgrade")
	TObjectPtr<UPangeaUpgradeSystemComponent> UpgradeSystem = nullptr;

	UPROPERTY(BlueprintReadOnly, Category="Upgrade")
	TObjectPtr<UPangeaFacilityManagerComponent> FacilityManager = nullptr;

	UObject* GetPreferredRequirementObject() const;
	UObject* GetPreferredActionObject() const;
};

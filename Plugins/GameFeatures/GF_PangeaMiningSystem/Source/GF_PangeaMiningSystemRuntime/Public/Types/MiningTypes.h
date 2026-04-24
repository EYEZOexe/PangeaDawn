#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "MiningTypes.generated.h"

class AActor;

USTRUCT(BlueprintType)
struct GF_PANGEAMININGSYSTEMRUNTIME_API FMiningItemQuantity
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mining")
	TSubclassOf<UObject> ItemClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mining")
	FGameplayTag ItemTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mining", meta=(ClampMin="0"))
	int32 Quantity = 0;
};

USTRUCT(BlueprintType)
struct GF_PANGEAMININGSYSTEMRUNTIME_API FMiningStoredItem
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category="Mining")
	TSubclassOf<UObject> ItemClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category="Mining")
	FGameplayTag ItemTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category="Mining", meta=(ClampMin="0"))
	int32 Quantity = 0;
};

USTRUCT(BlueprintType)
struct GF_PANGEAMININGSYSTEMRUNTIME_API FMiningSiteLevelDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mining|Level", meta=(ClampMin="0"))
	int32 Level = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mining|Level", meta=(ClampMin="0"))
	int32 StorageCapacity = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mining|Manual", meta=(ClampMin="0.0"))
	float ManualMiningSpeedMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mining|Automation", meta=(ClampMin="0"))
	int32 AutomatedMineralsPerDay = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mining|Automation", meta=(ClampMin="1"))
	int32 WorkerCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mining|Automation", meta=(ClampMin="0"))
	int32 GuardCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mining|Automation", meta=(ClampMin="0.0", ClampMax="1.0"))
	float WorkerDeathChance = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mining|Shipment")
	bool bShipmentUnlocked = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mining|Shipment", meta=(ClampMin="0"))
	int32 MaxShipmentUnits = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mining|Shipment", meta=(ClampMin="0.0"))
	float ShipmentIntervalSeconds = 3600.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mining|Shipment", meta=(ClampMin="0.0", ClampMax="1.0"))
	float ShipmentLossChance = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mining|Cost")
	TArray<FMiningItemQuantity> UpgradeCost;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mining|Output")
	FMiningItemQuantity PrimaryOutput;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mining|Visuals")
	TSoftClassPtr<AActor> VisualSetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mining|NPC")
	TSoftClassPtr<AActor> WorkerClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mining|NPC")
	TSoftClassPtr<AActor> GuardClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mining|NPC")
	TSoftClassPtr<AActor> CourierClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mining|NPC")
	TSoftObjectPtr<UObject> WorkerStateTreeAsset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mining|NPC")
	TSoftObjectPtr<UObject> CourierStateTreeAsset;
};

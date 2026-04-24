#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Items/ACFItem.h"
#include "MiningSettlementStockpileActor.generated.h"

class UACFStorageComponent;
class USceneComponent;
class UStaticMeshComponent;
class UTextRenderComponent;

UCLASS(BlueprintType, Blueprintable)
class GF_PANGEAMININGSYSTEMRUNTIME_API AMiningSettlementStockpileActor : public AActor
{
	GENERATED_BODY()

public:
	AMiningSettlementStockpileActor();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Mining")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Mining")
	TObjectPtr<UStaticMeshComponent> StockpileMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Mining")
	TObjectPtr<UTextRenderComponent> StockpileLabel;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Mining")
	TObjectPtr<USceneComponent> CourierUnloadMarker;

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Mining")
	TObjectPtr<UACFStorageComponent> StorageComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Mining")
	TArray<FBaseItem> InitialStock;

	FVector GetCourierUnloadLocation() const;
};

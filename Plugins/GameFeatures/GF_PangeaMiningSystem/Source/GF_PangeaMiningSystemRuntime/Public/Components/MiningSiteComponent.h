#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Types/MiningTypes.h"
#include "MiningSiteComponent.generated.h"

class FLifetimeProperty;
class UACFStorageComponent;
class UMiningSiteDefinition;

DECLARE_LOG_CATEGORY_EXTERN(LogPangeaMiningSite, Log, All);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FMiningSiteLevelChangedSignature, int32, OldLevel, int32, NewLevel);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FMiningSiteStorageChangedSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FMiningSiteShipmentResolvedSignature, const FMiningItemQuantity&, RequestedShipment, int32, DeliveredQuantity, bool, bLost);

UCLASS(ClassGroup=(Pangea), BlueprintType, Blueprintable, meta=(BlueprintSpawnableComponent))
class GF_PANGEAMININGSYSTEMRUNTIME_API UMiningSiteComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UMiningSiteComponent();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, SaveGame, Category="Mining")
	TObjectPtr<UMiningSiteDefinition> SiteDefinition;

	UPROPERTY(BlueprintAssignable, Category="Mining")
	FMiningSiteLevelChangedSignature OnSiteLevelChanged;

	UPROPERTY(BlueprintAssignable, Category="Mining")
	FMiningSiteStorageChangedSignature OnStorageChanged;

	UPROPERTY(BlueprintAssignable, Category="Mining")
	FMiningSiteShipmentResolvedSignature OnShipmentResolved;

	UFUNCTION(BlueprintPure, Category="Mining")
	bool IsEstablished() const { return bEstablished; }

	UFUNCTION(BlueprintPure, Category="Mining")
	int32 GetCurrentLevel() const { return CurrentLevel; }

	UFUNCTION(BlueprintPure, Category="Mining")
	int32 GetStorageCapacity() const;

	UFUNCTION(BlueprintPure, Category="Mining")
	int32 GetStoredUnits() const;

	UFUNCTION(BlueprintPure, Category="Mining")
	int32 GetAvailableStorage() const;

	UFUNCTION(BlueprintPure, Category="Mining")
	int32 GetStoredQuantity(const FMiningItemQuantity& Item) const;

	UFUNCTION(BlueprintPure, Category="Mining")
	float GetManualMiningSpeedMultiplier() const;

	UFUNCTION(BlueprintPure, Category="Mining")
	bool GetCurrentLevelDefinition(FMiningSiteLevelDefinition& OutDefinition) const;

	UFUNCTION(BlueprintPure, Category="Mining")
	bool GetNextLevelDefinition(FMiningSiteLevelDefinition& OutDefinition) const;

	UFUNCTION(BlueprintPure, Category="Mining")
	void GetNextUpgradeCost(TArray<FMiningItemQuantity>& OutCost) const;

	UFUNCTION(BlueprintCallable, Category="Mining")
	bool EstablishSite();

	UFUNCTION(BlueprintPure, Category="Mining")
	bool CanUpgradeSite() const;

	UFUNCTION(BlueprintCallable, Category="Mining")
	bool UpgradeSite();

	UFUNCTION(BlueprintPure, Category="Mining")
	bool CanPurchaseUpgrade(UObject* UpgradeContext) const;

	UFUNCTION(BlueprintCallable, Category="Mining")
	bool PurchaseUpgrade(UObject* UpgradeContext);

	UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category="Mining|Costs")
	bool CanPayUpgradeCost(UObject* UpgradeContext, const TArray<FMiningItemQuantity>& Cost) const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Mining|Costs")
	bool ConsumeUpgradeCost(UObject* UpgradeContext, const TArray<FMiningItemQuantity>& Cost);

	UFUNCTION(BlueprintCallable, Category="Mining")
	int32 AddStoredItem(const FMiningItemQuantity& Item);

	UFUNCTION(BlueprintCallable, Category="Mining")
	int32 RemoveStoredItem(const FMiningItemQuantity& Item);

	UFUNCTION(BlueprintCallable, Category="Mining")
	int32 ProduceForElapsedSeconds(float ElapsedSeconds);

	UFUNCTION(BlueprintCallable, Category="Mining")
	bool TryAutoShipment();

	UFUNCTION(BlueprintCallable, Category="Mining")
	void SyncProductionFromWorldTime();

	UFUNCTION(BlueprintCallable, Category="Mining|Storage")
	void SetLinkedStorageComponent(UACFStorageComponent* InStorageComponent);

protected:
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category="Mining")
	bool bEstablished = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category="Mining")
	int32 CurrentLevel = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category="Mining")
	TArray<FMiningStoredItem> StoredItems;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category="Mining")
	double LastProductionWorldSeconds = 0.0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category="Mining")
	double LastShipmentWorldSeconds = 0.0;

private:
	UPROPERTY(Transient)
	TObjectPtr<UACFStorageComponent> LinkedStorageComponent;

	bool HasAuthority() const;
	int32 FindStoredItemIndex(const FMiningItemQuantity& Item) const;
	bool CanStoreItem(const FMiningItemQuantity& Item) const;
	double GetWorldSeconds() const;
	UFUNCTION()
	void HandleLinkedStorageChanged();
	void RebuildStoredItemsFromLinkedStorage();
};

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MiningSettlementStockpileWidget.generated.h"

class AMiningSettlementStockpileActor;
class APawn;
class UButton;
class UACFInventoryComponent;
class UTextBlock;
class UVerticalBox;

UCLASS()
class MININGSYSTEMUI_API UMiningSettlementStockpileWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="Mining")
	void InitializeFromStockpile(AMiningSettlementStockpileActor* InStockpileActor, APawn* InInteractingPawn);

	void HandleTransferRequest(TSubclassOf<class UACFItem> ItemClass, int32 Count, bool bFromPlayerInventory);

protected:
	virtual void NativeOnInitialized() override;

private:
	void RebuildSection(UVerticalBox* Container, const TArray<struct FInventoryItem>& Items, bool bFromPlayerInventory);
	void BuildWidgetTree();
	void RefreshUI();
	void CloseMenu();
	FText BuildStatusText() const;

	UFUNCTION()
	void OnCloseClicked();

	UPROPERTY(Transient)
	TObjectPtr<AMiningSettlementStockpileActor> StockpileActor;

	UPROPERTY(Transient)
	TObjectPtr<APawn> InteractingPawn;

	UPROPERTY(Transient)
	TObjectPtr<UVerticalBox> RootBox;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> TitleText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> StatusText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> PlayerInventoryTitleText;

	UPROPERTY(Transient)
	TObjectPtr<UVerticalBox> PlayerItemsBox;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> StockpileInventoryTitleText;

	UPROPERTY(Transient)
	TObjectPtr<UVerticalBox> StockpileItemsBox;

	UPROPERTY(Transient)
	TObjectPtr<UButton> CloseButton;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> CloseButtonText;
};

#include "UI/MiningSettlementStockpileWidget.h"

#include "Actors/MiningSettlementStockpileActor.h"
#include "UI/MiningSettlementStockpileRowWidget.h"
#include "Components/ACFInventoryComponent.h"
#include "Components/ACFStorageComponent.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Blueprint/WidgetTree.h"
#include "GameFramework/PlayerController.h"
#include "Items/ACFItem.h"

void UMiningSettlementStockpileWidget::InitializeFromStockpile(AMiningSettlementStockpileActor* InStockpileActor, APawn* InInteractingPawn)
{
	StockpileActor = InStockpileActor;
	InteractingPawn = InInteractingPawn;
	RefreshUI();
}

void UMiningSettlementStockpileWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	BuildWidgetTree();
	RefreshUI();
}

void UMiningSettlementStockpileWidget::BuildWidgetTree()
{
	if (!WidgetTree || RootBox)
	{
		return;
	}

	RootBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RootBox"));
	WidgetTree->RootWidget = RootBox;

	auto AddText = [this](const TCHAR* Name, int32 FontSize) -> UTextBlock*
	{
		UTextBlock* TextBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
		TextBlock->SetAutoWrapText(true);
		FSlateFontInfo FontInfo = TextBlock->GetFont();
		FontInfo.Size = FontSize;
		TextBlock->SetFont(FontInfo);
		if (UVerticalBoxSlot* VerticalBoxSlot = RootBox->AddChildToVerticalBox(TextBlock))
		{
			VerticalBoxSlot->SetPadding(FMargin(8.0f));
		}
		return TextBlock;
	};

	auto AddBox = [this](const TCHAR* Name) -> UVerticalBox*
	{
		UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), Name);
		if (UVerticalBoxSlot* VerticalBoxSlot = RootBox->AddChildToVerticalBox(Box))
		{
			VerticalBoxSlot->SetPadding(FMargin(8.0f, 4.0f));
		}
		return Box;
	};

	TitleText = AddText(TEXT("TitleText"), 20);
	StatusText = AddText(TEXT("StatusText"), 14);
	PlayerInventoryTitleText = AddText(TEXT("PlayerInventoryTitleText"), 16);
	PlayerItemsBox = AddBox(TEXT("PlayerItemsBox"));
	StockpileInventoryTitleText = AddText(TEXT("StockpileInventoryTitleText"), 16);
	StockpileItemsBox = AddBox(TEXT("StockpileItemsBox"));

	CloseButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("CloseButton"));
	CloseButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CloseButtonText"));
	CloseButtonText->SetText(FText::FromString(TEXT("Close")));
	CloseButtonText->SetJustification(ETextJustify::Center);
	CloseButton->AddChild(CloseButtonText);
	if (UVerticalBoxSlot* VerticalBoxSlot = RootBox->AddChildToVerticalBox(CloseButton))
	{
		VerticalBoxSlot->SetPadding(FMargin(8.0f, 8.0f));
	}

	if (CloseButton)
	{
		CloseButton->OnClicked.AddDynamic(this, &ThisClass::OnCloseClicked);
	}
}

void UMiningSettlementStockpileWidget::RefreshUI()
{
	if (!TitleText || !StatusText || !PlayerInventoryTitleText || !PlayerItemsBox || !StockpileInventoryTitleText || !StockpileItemsBox)
	{
		return;
	}

	TitleText->SetText(FText::FromString(TEXT("Settlement Stockpile")));
	StatusText->SetText(BuildStatusText());
	PlayerInventoryTitleText->SetText(FText::FromString(TEXT("Player Inventory")));
	StockpileInventoryTitleText->SetText(FText::FromString(TEXT("Stockpile")));

	PlayerItemsBox->ClearChildren();
	StockpileItemsBox->ClearChildren();

	if (UACFInventoryComponent* InventoryComponent = InteractingPawn ? InteractingPawn->FindComponentByClass<UACFInventoryComponent>() : nullptr)
	{
		RebuildSection(PlayerItemsBox, InventoryComponent->GetInventory(), true);
	}

	if (StockpileActor && StockpileActor->StorageComponent)
	{
		RebuildSection(StockpileItemsBox, StockpileActor->StorageComponent->GetInventory(), false);
	}
}

void UMiningSettlementStockpileWidget::RebuildSection(UVerticalBox* Container, const TArray<FInventoryItem>& Items, bool bFromPlayerInventory)
{
	if (!WidgetTree || !Container)
	{
		return;
	}

	if (Items.IsEmpty())
	{
		UTextBlock* EmptyText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), bFromPlayerInventory ? TEXT("EmptyPlayerText") : TEXT("EmptyStockpileText"));
		EmptyText->SetText(FText::FromString(TEXT("Empty")));
		if (UVerticalBoxSlot* VerticalBoxSlot = Container->AddChildToVerticalBox(EmptyText))
		{
			VerticalBoxSlot->SetPadding(FMargin(4.0f));
		}
		return;
	}

	for (const FInventoryItem& Item : Items)
	{
		if (!Item.ItemClass || Item.Count <= 0)
		{
			continue;
		}

		const FString ItemName = Item.ItemClass->GetDefaultObject<UACFItem>()
			? Item.ItemClass->GetDefaultObject<UACFItem>()->GetItemName().ToString()
			: Item.ItemClass->GetName();
		const FString Label = FString::Printf(TEXT("%s %s x%d"), bFromPlayerInventory ? TEXT("Deposit") : TEXT("Withdraw"), *ItemName, Item.Count);

		UMiningSettlementStockpileRowWidget* RowWidget = CreateWidget<UMiningSettlementStockpileRowWidget>(GetOwningPlayer(), UMiningSettlementStockpileRowWidget::StaticClass());
		if (!RowWidget)
		{
			continue;
		}

		RowWidget->InitializeRow(this, Item.ItemClass, Item.Count, bFromPlayerInventory, Label);
		if (UVerticalBoxSlot* VerticalBoxSlot = Container->AddChildToVerticalBox(RowWidget))
		{
			VerticalBoxSlot->SetPadding(FMargin(4.0f, 2.0f));
		}
	}
}

FText UMiningSettlementStockpileWidget::BuildStatusText() const
{
	if (!StockpileActor || !StockpileActor->StorageComponent)
	{
		return FText::FromString(TEXT("No stockpile storage."));
	}

	int32 StoredCount = 0;
	for (const FInventoryItem& Item : StockpileActor->StorageComponent->GetInventory())
	{
		StoredCount += Item.Count;
	}

	return FText::FromString(FString::Printf(TEXT("Stored item count: %d"), StoredCount));
}

void UMiningSettlementStockpileWidget::CloseMenu()
{
	if (StockpileActor)
	{
		StockpileActor->ClearActiveStockpileWidget(this);
	}

	RemoveFromParent();

	if (APlayerController* PlayerController = Cast<APlayerController>(InteractingPawn ? InteractingPawn->GetController() : nullptr))
	{
		PlayerController->bShowMouseCursor = false;
		PlayerController->SetInputMode(FInputModeGameOnly());
	}
}

void UMiningSettlementStockpileWidget::HandleTransferRequest(TSubclassOf<UACFItem> ItemClass, int32 Count, bool bFromPlayerInventory)
{
	if (!StockpileActor || !InteractingPawn || !ItemClass || Count <= 0)
	{
		return;
	}

	if (bFromPlayerInventory)
	{
		StockpileActor->ServerTransferItemFromPawn(InteractingPawn, ItemClass, Count);
	}
	else
	{
		StockpileActor->ServerTransferItemToPawn(InteractingPawn, ItemClass, Count);
	}

	RefreshUI();
}

void UMiningSettlementStockpileWidget::OnCloseClicked()
{
	CloseMenu();
}

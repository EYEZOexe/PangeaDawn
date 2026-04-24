#include "Components/MiningSiteComponent.h"

#include "Components/ACFStorageComponent.h"
#include "DataAssets/MiningSiteDefinition.h"
#include "Items/ACFItem.h"

namespace
{
TSubclassOf<UACFItem> ResolveACFItemClass(const FMiningItemQuantity& Item)
{
	UClass* RawItemClass = Item.ItemClass.Get();
	return RawItemClass && RawItemClass->IsChildOf(UACFItem::StaticClass()) ? RawItemClass : nullptr;
}
}

int32 UMiningSiteComponent::AddStoredItem(const FMiningItemQuantity& Item)
{
	if (!HasAuthority() || Item.Quantity <= 0 || !CanStoreItem(Item))
	{
		UE_LOG(LogPangeaMiningSite, Verbose, TEXT("AddStoredItem blocked. Owner=%s Quantity=%d Available=%d"),
			*GetNameSafe(GetOwner()),
			Item.Quantity,
			GetAvailableStorage());
		return 0;
	}

	const int32 AcceptedQuantity = FMath::Min(Item.Quantity, GetAvailableStorage());
	if (AcceptedQuantity <= 0)
	{
		return 0;
	}

	if (LinkedStorageComponent)
	{
		FMiningItemQuantity AcceptedItem = Item;
		AcceptedItem.Quantity = AcceptedQuantity;
		if (TSubclassOf<UACFItem> ItemClass = ResolveACFItemClass(AcceptedItem))
		{
			LinkedStorageComponent->AddItem(FBaseItem(ItemClass, AcceptedQuantity));
			RebuildStoredItemsFromLinkedStorage();
			UE_LOG(LogPangeaMiningSite, Log, TEXT("Mining storage added %d/%d units via linked chest. Owner=%s Stored=%d Capacity=%d ItemClass=%s Tag=%s"),
				AcceptedQuantity,
				Item.Quantity,
				*GetNameSafe(GetOwner()),
				GetStoredUnits(),
				GetStorageCapacity(),
				*GetNameSafe(Item.ItemClass.Get()),
				*Item.ItemTag.ToString());
			return AcceptedQuantity;
		}
	}

	const int32 ExistingIndex = FindStoredItemIndex(Item);
	if (StoredItems.IsValidIndex(ExistingIndex))
	{
		StoredItems[ExistingIndex].Quantity += AcceptedQuantity;
	}
	else
	{
		FMiningStoredItem StoredItem;
		StoredItem.ItemClass = Item.ItemClass;
		StoredItem.ItemTag = Item.ItemTag;
		StoredItem.Quantity = AcceptedQuantity;
		StoredItems.Add(StoredItem);
	}

	OnStorageChanged.Broadcast();
	UE_LOG(LogPangeaMiningSite, Log, TEXT("Mining storage added %d/%d units. Owner=%s Stored=%d Capacity=%d ItemClass=%s Tag=%s"),
		AcceptedQuantity,
		Item.Quantity,
		*GetNameSafe(GetOwner()),
		GetStoredUnits(),
		GetStorageCapacity(),
		*GetNameSafe(Item.ItemClass.Get()),
		*Item.ItemTag.ToString());
	return AcceptedQuantity;
}

int32 UMiningSiteComponent::RemoveStoredItem(const FMiningItemQuantity& Item)
{
	if (!HasAuthority() || Item.Quantity <= 0)
	{
		return 0;
	}

	if (LinkedStorageComponent)
	{
		if (TSubclassOf<UACFItem> ItemClass = ResolveACFItemClass(Item))
		{
			int32 RemainingToRemove = Item.Quantity;
			TArray<FInventoryItem> MatchingItems;
			LinkedStorageComponent->GetAllItemsOfClassInInventory(ItemClass, MatchingItems);

			for (const FInventoryItem& InventoryItem : MatchingItems)
			{
				if (RemainingToRemove <= 0)
				{
					break;
				}

				const int32 AmountToRemove = FMath::Min(RemainingToRemove, InventoryItem.Count);
				if (AmountToRemove > 0)
				{
					LinkedStorageComponent->RemoveItem(InventoryItem, AmountToRemove);
					RemainingToRemove -= AmountToRemove;
				}
			}

			RebuildStoredItemsFromLinkedStorage();
			return Item.Quantity - RemainingToRemove;
		}
	}

	const int32 ExistingIndex = FindStoredItemIndex(Item);
	if (!StoredItems.IsValidIndex(ExistingIndex))
	{
		return 0;
	}

	const int32 RemovedQuantity = FMath::Min(Item.Quantity, StoredItems[ExistingIndex].Quantity);
	StoredItems[ExistingIndex].Quantity -= RemovedQuantity;
	if (StoredItems[ExistingIndex].Quantity <= 0)
	{
		StoredItems.RemoveAt(ExistingIndex);
	}

	OnStorageChanged.Broadcast();
	return RemovedQuantity;
}

int32 UMiningSiteComponent::ProduceForElapsedSeconds(const float ElapsedSeconds)
{
	if (!HasAuthority() || !bEstablished || ElapsedSeconds <= 0.0f)
	{
		UE_LOG(LogPangeaMiningSite, Verbose, TEXT("ProduceForElapsedSeconds blocked. Owner=%s Established=%s Elapsed=%.2f"),
			*GetNameSafe(GetOwner()),
			bEstablished ? TEXT("true") : TEXT("false"),
			ElapsedSeconds);
		return 0;
	}

	FMiningSiteLevelDefinition Definition;
	if (!GetCurrentLevelDefinition(Definition) || Definition.AutomatedMineralsPerDay <= 0 || !SiteDefinition)
	{
		UE_LOG(LogPangeaMiningSite, Verbose, TEXT("No mining automation. Owner=%s Level=%d AutoPerDay=%d"),
			*GetNameSafe(GetOwner()),
			CurrentLevel,
			Definition.AutomatedMineralsPerDay);
		return 0;
	}

	const float DaySeconds = FMath::Max(1.0f, SiteDefinition->SimulatedDaySeconds);
	const int32 ProducedQuantity = FMath::FloorToInt((ElapsedSeconds / DaySeconds) * Definition.AutomatedMineralsPerDay);
	if (ProducedQuantity <= 0)
	{
		return 0;
	}

	FMiningItemQuantity ProducedItem = Definition.PrimaryOutput;
	ProducedItem.Quantity = ProducedQuantity;
	const int32 AcceptedQuantity = AddStoredItem(ProducedItem);
	UE_LOG(LogPangeaMiningSite, Log, TEXT("Mining production tick. Owner=%s Elapsed=%.2f Requested=%d Accepted=%d"),
		*GetNameSafe(GetOwner()),
		ElapsedSeconds,
		ProducedQuantity,
		AcceptedQuantity);
	return AcceptedQuantity;
}

bool UMiningSiteComponent::TryAutoShipment()
{
	if (!HasAuthority() || !bEstablished)
	{
		UE_LOG(LogPangeaMiningSite, Verbose, TEXT("TryAutoShipment blocked. Owner=%s Established=%s"),
			*GetNameSafe(GetOwner()),
			bEstablished ? TEXT("true") : TEXT("false"));
		return false;
	}

	FMiningSiteLevelDefinition Definition;
	if (!GetCurrentLevelDefinition(Definition) || !Definition.bShipmentUnlocked || Definition.MaxShipmentUnits <= 0)
	{
		UE_LOG(LogPangeaMiningSite, Verbose, TEXT("Shipment unavailable. Owner=%s Level=%d Unlocked=%s MaxShipment=%d"),
			*GetNameSafe(GetOwner()),
			CurrentLevel,
			Definition.bShipmentUnlocked ? TEXT("true") : TEXT("false"),
			Definition.MaxShipmentUnits);
		return false;
	}

	const double CurrentWorldSeconds = GetWorldSeconds();
	if (LastShipmentWorldSeconds > 0.0 && CurrentWorldSeconds - LastShipmentWorldSeconds < Definition.ShipmentIntervalSeconds)
	{
		return false;
	}

	FMiningItemQuantity ShipmentItem = Definition.PrimaryOutput;
	ShipmentItem.Quantity = FMath::Min(Definition.MaxShipmentUnits, GetStoredQuantity(ShipmentItem));
	if (ShipmentItem.Quantity <= 0)
	{
		LastShipmentWorldSeconds = CurrentWorldSeconds;
		return false;
	}

	const int32 RemovedQuantity = RemoveStoredItem(ShipmentItem);
	const bool bLost = FMath::FRand() < Definition.ShipmentLossChance;
	const int32 DeliveredQuantity = bLost ? 0 : RemovedQuantity;
	LastShipmentWorldSeconds = CurrentWorldSeconds;
	OnShipmentResolved.Broadcast(ShipmentItem, DeliveredQuantity, bLost);
	UE_LOG(LogPangeaMiningSite, Log, TEXT("Mining shipment resolved. Owner=%s Removed=%d Delivered=%d Lost=%s LossChance=%.2f"),
		*GetNameSafe(GetOwner()),
		RemovedQuantity,
		DeliveredQuantity,
		bLost ? TEXT("true") : TEXT("false"),
		Definition.ShipmentLossChance);
	return RemovedQuantity > 0;
}

void UMiningSiteComponent::SyncProductionFromWorldTime()
{
	if (!HasAuthority() || !bEstablished)
	{
		return;
	}

	const double CurrentWorldSeconds = GetWorldSeconds();
	if (LastProductionWorldSeconds <= 0.0)
	{
		LastProductionWorldSeconds = CurrentWorldSeconds;
		return;
	}

	ProduceForElapsedSeconds(static_cast<float>(CurrentWorldSeconds - LastProductionWorldSeconds));
	LastProductionWorldSeconds = CurrentWorldSeconds;
	TryAutoShipment();
	UE_LOG(LogPangeaMiningSite, Log, TEXT("Mining site synced from world time. Owner=%s Stored=%d Capacity=%d"),
		*GetNameSafe(GetOwner()),
		GetStoredUnits(),
		GetStorageCapacity());
}

void UMiningSiteComponent::SetLinkedStorageComponent(UACFStorageComponent* InStorageComponent)
{
	if (LinkedStorageComponent == InStorageComponent)
	{
		return;
	}

	if (LinkedStorageComponent && LinkedStorageComponent->OnInventoryChanged.IsAlreadyBound(this, &ThisClass::HandleLinkedStorageChanged))
	{
		LinkedStorageComponent->OnInventoryChanged.RemoveDynamic(this, &ThisClass::HandleLinkedStorageChanged);
	}

	LinkedStorageComponent = InStorageComponent;

	if (!LinkedStorageComponent)
	{
		return;
	}

	if (!LinkedStorageComponent->OnInventoryChanged.IsAlreadyBound(this, &ThisClass::HandleLinkedStorageChanged))
	{
		LinkedStorageComponent->OnInventoryChanged.AddDynamic(this, &ThisClass::HandleLinkedStorageChanged);
	}

	if (LinkedStorageComponent->GetInventory().IsEmpty())
	{
		for (const FMiningStoredItem& StoredItem : StoredItems)
		{
			FMiningItemQuantity MiningItem;
			MiningItem.ItemClass = StoredItem.ItemClass;
			MiningItem.ItemTag = StoredItem.ItemTag;
			MiningItem.Quantity = StoredItem.Quantity;

			if (TSubclassOf<UACFItem> ItemClass = ResolveACFItemClass(MiningItem))
			{
				LinkedStorageComponent->AddItem(FBaseItem(ItemClass, StoredItem.Quantity));
			}
		}
	}

	RebuildStoredItemsFromLinkedStorage();
}

void UMiningSiteComponent::RebuildStoredItemsFromLinkedStorage()
{
	if (!LinkedStorageComponent)
	{
		return;
	}

	StoredItems.Reset();

	for (const FInventoryItem& InventoryItem : LinkedStorageComponent->GetInventory())
	{
		if (!InventoryItem.ItemClass || InventoryItem.Count <= 0)
		{
			continue;
		}

		FMiningStoredItem StoredItem;
		StoredItem.ItemClass = InventoryItem.ItemClass;
		StoredItem.Quantity = InventoryItem.Count;
		StoredItems.Add(StoredItem);
	}

	OnStorageChanged.Broadcast();
}

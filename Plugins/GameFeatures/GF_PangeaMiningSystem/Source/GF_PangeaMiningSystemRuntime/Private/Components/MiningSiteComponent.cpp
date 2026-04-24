#include "Components/MiningSiteComponent.h"

#include "Components/ACFStorageComponent.h"
#include "DataAssets/MiningSiteDefinition.h"
#include "Net/UnrealNetwork.h"

DEFINE_LOG_CATEGORY(LogPangeaMiningSite);

UMiningSiteComponent::UMiningSiteComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UMiningSiteComponent::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority() && bEstablished && LastProductionWorldSeconds <= 0.0)
	{
		LastProductionWorldSeconds = GetWorldSeconds();
		LastShipmentWorldSeconds = LastProductionWorldSeconds;
	}
}

int32 UMiningSiteComponent::GetStorageCapacity() const
{
	FMiningSiteLevelDefinition Definition;
	return GetCurrentLevelDefinition(Definition) ? Definition.StorageCapacity : 0;
}

int32 UMiningSiteComponent::GetStoredUnits() const
{
	int32 StoredUnits = 0;
	for (const FMiningStoredItem& Item : StoredItems)
	{
		StoredUnits += FMath::Max(0, Item.Quantity);
	}

	return StoredUnits;
}

int32 UMiningSiteComponent::GetAvailableStorage() const
{
	return FMath::Max(0, GetStorageCapacity() - GetStoredUnits());
}

int32 UMiningSiteComponent::GetStoredQuantity(const FMiningItemQuantity& Item) const
{
	const int32 Index = FindStoredItemIndex(Item);
	return StoredItems.IsValidIndex(Index) ? StoredItems[Index].Quantity : 0;
}

float UMiningSiteComponent::GetManualMiningSpeedMultiplier() const
{
	FMiningSiteLevelDefinition Definition;
	return GetCurrentLevelDefinition(Definition) ? Definition.ManualMiningSpeedMultiplier : 1.0f;
}

bool UMiningSiteComponent::GetCurrentLevelDefinition(FMiningSiteLevelDefinition& OutDefinition) const
{
	return SiteDefinition ? SiteDefinition->GetLevelDefinition(CurrentLevel, OutDefinition) : false;
}

bool UMiningSiteComponent::GetNextLevelDefinition(FMiningSiteLevelDefinition& OutDefinition) const
{
	return SiteDefinition ? SiteDefinition->GetLevelDefinition(CurrentLevel + 1, OutDefinition) : false;
}

void UMiningSiteComponent::GetNextUpgradeCost(TArray<FMiningItemQuantity>& OutCost) const
{
	OutCost.Reset();

	FMiningSiteLevelDefinition NextLevel;
	if (GetNextLevelDefinition(NextLevel))
	{
		OutCost = NextLevel.UpgradeCost;
	}
}

bool UMiningSiteComponent::HasAuthority() const
{
	const AActor* Owner = GetOwner();
	return Owner && Owner->HasAuthority();
}

int32 UMiningSiteComponent::FindStoredItemIndex(const FMiningItemQuantity& Item) const
{
	for (int32 Index = 0; Index < StoredItems.Num(); ++Index)
	{
		const FMiningStoredItem& StoredItem = StoredItems[Index];
		const bool bClassMatches = Item.ItemClass && StoredItem.ItemClass == Item.ItemClass;
		const bool bTagMatches = Item.ItemTag.IsValid() && StoredItem.ItemTag == Item.ItemTag;
		if (bClassMatches || bTagMatches)
		{
			return Index;
		}
	}

	return INDEX_NONE;
}

bool UMiningSiteComponent::CanStoreItem(const FMiningItemQuantity& Item) const
{
	if (!SiteDefinition || !Item.ItemTag.IsValid() || SiteDefinition->AllowedStorageItemTags.IsEmpty())
	{
		return true;
	}

	for (const FGameplayTag& AllowedTag : SiteDefinition->AllowedStorageItemTags)
	{
		if (Item.ItemTag.MatchesTag(AllowedTag))
		{
			return true;
		}
	}

	return false;
}

double UMiningSiteComponent::GetWorldSeconds() const
{
	const UWorld* World = GetWorld();
	return World ? World->GetTimeSeconds() : 0.0;
}

void UMiningSiteComponent::HandleLinkedStorageChanged()
{
	RebuildStoredItemsFromLinkedStorage();
}

void UMiningSiteComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UMiningSiteComponent, bEstablished);
	DOREPLIFETIME(UMiningSiteComponent, SiteDefinition);
	DOREPLIFETIME(UMiningSiteComponent, CurrentLevel);
	DOREPLIFETIME(UMiningSiteComponent, StoredItems);
	DOREPLIFETIME(UMiningSiteComponent, LastProductionWorldSeconds);
	DOREPLIFETIME(UMiningSiteComponent, LastShipmentWorldSeconds);
}

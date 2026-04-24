#include "Actors/MiningSettlementStockpileActor.h"

#include "Components/ACFStorageComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "UObject/ConstructorHelpers.h"

DEFINE_LOG_CATEGORY_STATIC(LogPangeaMiningStockpile, Log, All);

AMiningSettlementStockpileActor::AMiningSettlementStockpileActor()
{
	bReplicates = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	StockpileMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StockpileMesh"));
	StockpileMesh->SetupAttachment(SceneRoot);
	StockpileMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

	StockpileLabel = CreateDefaultSubobject<UTextRenderComponent>(TEXT("StockpileLabel"));
	StockpileLabel->SetupAttachment(SceneRoot);
	StockpileLabel->SetRelativeLocation(FVector(0.0f, 0.0f, 180.0f));
	StockpileLabel->SetHorizontalAlignment(EHorizTextAligment::EHTA_Center);
	StockpileLabel->SetWorldSize(28.0f);
	StockpileLabel->SetText(FText::FromString(TEXT("Settlement Stockpile")));

	CourierUnloadMarker = CreateDefaultSubobject<USceneComponent>(TEXT("CourierUnloadMarker"));
	CourierUnloadMarker->SetupAttachment(SceneRoot);
	CourierUnloadMarker->SetRelativeLocation(FVector(120.0f, 0.0f, 0.0f));

	StorageComponent = CreateDefaultSubobject<UACFStorageComponent>(TEXT("StorageComponent"));
	if (StorageComponent)
	{
		StorageComponent->SetMaxInventorySlots(1000);
		StorageComponent->SetMaxInventoryWeight(1000000);
	}

	static ConstructorHelpers::FObjectFinder<UStaticMesh> ChestAsset(TEXT("/Game/Environment/Oppidam/Decoration/SM_Oppi_Deco_Chest_AD.SM_Oppi_Deco_Chest_AD"));
	if (ChestAsset.Succeeded())
	{
		StockpileMesh->SetStaticMesh(ChestAsset.Object);
		StockpileMesh->SetRelativeScale3D(FVector(1.5f));
	}
}

void AMiningSettlementStockpileActor::BeginPlay()
{
	Super::BeginPlay();

	if (!HasAuthority() || !StorageComponent || !StorageComponent->GetInventory().IsEmpty())
	{
		return;
	}

	for (const FBaseItem& Item : InitialStock)
	{
		if (Item.ItemClass && Item.Count > 0)
		{
			StorageComponent->AddItem(Item);
		}
	}

	UE_LOG(LogPangeaMiningStockpile, Log, TEXT("Settlement stockpile initialized. Actor=%s ItemStacks=%d"),
		*GetNameSafe(this),
		StorageComponent->GetInventory().Num());

	for (const FInventoryItem& Item : StorageComponent->GetInventory())
	{
		UE_LOG(LogPangeaMiningStockpile, Log, TEXT("Settlement stockpile item. Actor=%s ItemClass=%s Count=%d"),
			*GetNameSafe(this),
			*GetNameSafe(Item.ItemClass.Get()),
			Item.Count);
	}
}

FVector AMiningSettlementStockpileActor::GetCourierUnloadLocation() const
{
	return CourierUnloadMarker ? CourierUnloadMarker->GetComponentLocation() : GetActorLocation();
}

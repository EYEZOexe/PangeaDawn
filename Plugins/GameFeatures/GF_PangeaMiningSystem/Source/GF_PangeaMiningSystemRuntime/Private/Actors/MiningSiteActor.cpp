#include "Actors/MiningSiteActor.h"

#include "Components/ACFInteractionComponent.h"
#include "Components/ACFInventoryComponent.h"
#include "Components/ACFStorageComponent.h"
#include "Components/MiningSiteComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SceneComponent.h"
#include "DataAssets/MiningSiteDefinition.h"
#include "Blueprint/UserWidget.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Interfaces/MiningSitePresentationCoordinatorInterface.h"
#include "SmartObjectComponent.h"
#include "TimerManager.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

DEFINE_LOG_CATEGORY_STATIC(LogPangeaMiningSiteActor, Log, All);

AMiningSiteActor::AMiningSiteActor()
{
	bReplicates = true;
	PrimaryActorTick.bCanEverTick = false;
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	SiteMarkerMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SiteMarkerMesh"));
	SiteMarkerMesh->SetupAttachment(SceneRoot);
	SiteMarkerMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

	InteractionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionSphere"));
	InteractionSphere->SetupAttachment(SceneRoot);
	InteractionSphere->SetSphereRadius(150.0f);
	InteractionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	InteractionSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	InteractionSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	StatusText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("StatusText"));
	StatusText->SetupAttachment(SceneRoot);
	StatusText->SetRelativeLocation(FVector(0.0f, 0.0f, 240.0f));
	StatusText->SetHorizontalAlignment(EHorizTextAligment::EHTA_Center);
	StatusText->SetWorldSize(28.0f);
	StatusText->SetText(FText::FromString(TEXT("Mining Site")));

	UStaticMeshComponent* FireMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DebugCampfireMesh"));
	FireMesh->SetupAttachment(SceneRoot);
	FireMesh->SetRelativeLocation(FVector(-100.0f, 80.0f, 0.0f));
	FireMesh->SetRelativeScale3D(FVector(1.0f));
	FireMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

	UStaticMeshComponent* WorkstationMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DebugWorkstationMesh"));
	WorkstationMesh->SetupAttachment(SceneRoot);
	WorkstationMesh->SetRelativeLocation(FVector(-120.0f, -90.0f, 0.0f));
	WorkstationMesh->SetRelativeScale3D(FVector(0.6f));
	WorkstationMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> FireAsset(TEXT("/Game/Environment/Oppidam/Decoration/SM_Oppi_Deco_fire_camp_AB.SM_Oppi_Deco_fire_camp_AB"));
	if (FireAsset.Succeeded())
	{
		FireMesh->SetStaticMesh(FireAsset.Object);
	}

	static ConstructorHelpers::FObjectFinder<UStaticMesh> WorkstationAsset(TEXT("/Game/FullSample/Assets/Environment/Meshes/Crops/SM_Stone_03.SM_Stone_03"));
	if (WorkstationAsset.Succeeded())
	{
		WorkstationMesh->SetStaticMesh(WorkstationAsset.Object);
	}

	MiningSiteComponent = CreateDefaultSubobject<UMiningSiteComponent>(TEXT("MiningSiteComponent"));
}

void AMiningSiteActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AMiningSiteActor, SettlementResourceActor);
}

void AMiningSiteActor::BeginPlay()
{
	Super::BeginPlay();

	if (MiningSiteComponent)
	{
		MiningSiteComponent->OnSiteLevelChanged.AddDynamic(this, &ThisClass::HandleSiteLevelChanged);
		MiningSiteComponent->OnStorageChanged.AddDynamic(this, &ThisClass::UpdateStatusText);
		MiningSiteComponent->OnShipmentResolved.AddDynamic(this, &ThisClass::HandleShipmentResolved);
	}

	InteractionSphere->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::HandleInteractionBegin);
	InteractionSphere->OnComponentEndOverlap.AddDynamic(this, &ThisClass::HandleInteractionEnd);
	RefreshLocalInteractionRegistration();

	UE_LOG(LogPangeaMiningSiteActor, Log, TEXT("Mining site actor BeginPlay. Actor=%s HasAuthority=%s Component=%s Definition=%s Settlement=%s Established=%s Level=%d Location=%s"),
		*GetNameSafe(this),
		HasAuthority() ? TEXT("true") : TEXT("false"),
		*GetNameSafe(MiningSiteComponent),
		MiningSiteComponent ? *GetNameSafe(MiningSiteComponent->SiteDefinition) : TEXT("None"),
		*GetNameSafe(SettlementResourceActor),
		MiningSiteComponent && MiningSiteComponent->IsEstablished() ? TEXT("true") : TEXT("false"),
		MiningSiteComponent ? MiningSiteComponent->GetCurrentLevel() : INDEX_NONE,
		*GetActorLocation().ToString());

	RefreshLevelVisuals();
	RefreshSiteChest();
	EnsurePresentationCoordinator();
	ConfigureSmartObjectComponents();
	RefreshPresentationActors();
	UpdatePresentationActorMovement();
	UpdateStatusText();

	if (HasAuthority())
	{
		const IMiningSitePresentationCoordinatorInterface* Coordinator = Cast<IMiningSitePresentationCoordinatorInterface>(PresentationCoordinator);
		const float RefreshInterval = Coordinator ? Coordinator->GetPresentationRefreshInterval() : 1.0f;
		const float MovementInterval = Coordinator ? Coordinator->GetPresentationMovementInterval() : 0.033f;
		GetWorldTimerManager().SetTimer(PresentationRefreshTimerHandle, this, &ThisClass::RefreshPresentationActors, RefreshInterval, true);
		GetWorldTimerManager().SetTimer(PresentationMovementTimerHandle, this, &ThisClass::UpdatePresentationActorMovement, MovementInterval, true);
	}
}

void AMiningSiteActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PresentationRefreshTimerHandle);
		World->GetTimerManager().ClearTimer(PresentationMovementTimerHandle);
	}

	ClearPresentationActors();

	for (const TWeakObjectPtr<UACFInteractionComponent>& InteractionComponent : RegisteredInteractionComponents)
	{
		if (InteractionComponent.IsValid())
		{
			InteractionComponent->UnregisterInteractable(this);
		}
	}

	RegisteredInteractionComponents.Reset();
	Super::EndPlay(EndPlayReason);
}

void AMiningSiteActor::RefreshLevelVisuals()
{
	if (!HasAuthority() || !MiningSiteComponent || !MiningSiteComponent->IsEstablished())
	{
		UE_LOG(LogPangeaMiningSiteActor, Log, TEXT("RefreshLevelVisuals skipped. Actor=%s HasAuthority=%s Component=%s Established=%s"),
			*GetNameSafe(this),
			HasAuthority() ? TEXT("true") : TEXT("false"),
			*GetNameSafe(MiningSiteComponent),
			MiningSiteComponent && MiningSiteComponent->IsEstablished() ? TEXT("true") : TEXT("false"));
		return;
	}

	if (SpawnedLevelVisuals)
	{
		SpawnedLevelVisuals->Destroy();
		SpawnedLevelVisuals = nullptr;
	}

	FMiningSiteLevelDefinition LevelDefinition;
	if (!MiningSiteComponent->GetCurrentLevelDefinition(LevelDefinition) || LevelDefinition.VisualSetClass.IsNull())
	{
		UE_LOG(LogPangeaMiningSiteActor, Log, TEXT("No visual set configured. Actor=%s Level=%d"),
			*GetNameSafe(this),
			MiningSiteComponent->GetCurrentLevel());
		return;
	}

	UClass* VisualClass = LevelDefinition.VisualSetClass.LoadSynchronous();
	if (!VisualClass)
	{
		UE_LOG(LogPangeaMiningSiteActor, Warning, TEXT("Failed to load visual set. Actor=%s Level=%d"),
			*GetNameSafe(this),
			MiningSiteComponent->GetCurrentLevel());
		return;
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = this;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	SpawnedLevelVisuals = GetWorld()->SpawnActor<AActor>(VisualClass, GetActorTransform(), SpawnParameters);
	if (SpawnedLevelVisuals)
	{
		SpawnedLevelVisuals->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
		UE_LOG(LogPangeaMiningSiteActor, Log, TEXT("Spawned mining visual set. Actor=%s Visual=%s Level=%d"),
			*GetNameSafe(this),
			*GetNameSafe(SpawnedLevelVisuals),
			MiningSiteComponent->GetCurrentLevel());
	}

	UpdateStatusText();
}

void AMiningSiteActor::RefreshSiteChest()
{
	if (!HasAuthority() || !MiningSiteComponent || !MiningSiteComponent->IsEstablished() || !MiningSiteComponent->SiteDefinition)
	{
		return;
	}

	if (SpawnedSiteChest)
	{
		return;
	}

	if (MiningSiteComponent->SiteDefinition->SiteChestActorClass.IsNull())
	{
		return;
	}

	UClass* ChestClass = MiningSiteComponent->SiteDefinition->SiteChestActorClass.LoadSynchronous();
	const FTransform RelativeTransform = MiningSiteComponent->SiteDefinition->SiteChestRelativeTransform;

	if (!ChestClass)
	{
		UE_LOG(LogPangeaMiningSiteActor, Warning, TEXT("Failed to load site chest class. Actor=%s Definition=%s"),
			*GetNameSafe(this),
			*GetNameSafe(MiningSiteComponent->SiteDefinition));
		return;
	}

	const FTransform SpawnTransform = RelativeTransform * GetActorTransform();

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = this;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	SpawnedSiteChest = GetWorld()->SpawnActor<AActor>(ChestClass, SpawnTransform, SpawnParameters);
	if (SpawnedSiteChest)
	{
		SpawnedSiteChest->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);

		TArray<UStaticMeshComponent*> ExistingStaticMeshComponents;
		SpawnedSiteChest->GetComponents<UStaticMeshComponent>(ExistingStaticMeshComponents);
		if (ExistingStaticMeshComponents.IsEmpty())
		{
			UStaticMeshComponent* ChestVisualMesh = NewObject<UStaticMeshComponent>(SpawnedSiteChest, TEXT("MiningSiteChestVisualMesh"));
			if (ChestVisualMesh)
			{
				ChestVisualMesh->SetupAttachment(SpawnedSiteChest->GetRootComponent());
				ChestVisualMesh->SetRelativeLocation(FVector::ZeroVector);
				ChestVisualMesh->SetRelativeRotation(FRotator::ZeroRotator);
				ChestVisualMesh->SetRelativeScale3D(FVector(0.9f));
				ChestVisualMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

				if (UStaticMesh* ChestMeshAsset = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Environment/Oppidam/Decoration/SM_Oppi_Deco_Chest_AD.SM_Oppi_Deco_Chest_AD")))
				{
					ChestVisualMesh->SetStaticMesh(ChestMeshAsset);
				}

				ChestVisualMesh->RegisterComponent();
			}
		}

		if (MiningSiteComponent)
		{
			ClearSpawnedSiteChestInventory();
			MiningSiteComponent->SetLinkedStorageComponent(SpawnedSiteChest->FindComponentByClass<UACFStorageComponent>());
		}

		SpawnedSiteChestInteractionSphere = NewObject<USphereComponent>(SpawnedSiteChest, TEXT("MiningSiteChestInteractionSphere"));
		if (SpawnedSiteChestInteractionSphere)
		{
			SpawnedSiteChestInteractionSphere->SetupAttachment(SpawnedSiteChest->GetRootComponent());
			SpawnedSiteChestInteractionSphere->SetSphereRadius(140.0f);
			SpawnedSiteChestInteractionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
			SpawnedSiteChestInteractionSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
			SpawnedSiteChestInteractionSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
			SpawnedSiteChestInteractionSphere->RegisterComponent();
			SpawnedSiteChestInteractionSphere->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::HandleSiteChestInteractionBegin);
			SpawnedSiteChestInteractionSphere->OnComponentEndOverlap.AddDynamic(this, &ThisClass::HandleSiteChestInteractionEnd);
		}

		UE_LOG(LogPangeaMiningSiteActor, Log, TEXT("Spawned mining site chest. Actor=%s Chest=%s ChestClass=%s"),
			*GetNameSafe(this),
			*GetNameSafe(SpawnedSiteChest),
			*GetNameSafe(ChestClass));
	}
}

void AMiningSiteActor::ClearSpawnedSiteChestInventory()
{
	UACFStorageComponent* StorageComponent = SpawnedSiteChest ? SpawnedSiteChest->FindComponentByClass<UACFStorageComponent>() : nullptr;
	if (!StorageComponent)
	{
		return;
	}

	const TArray<FInventoryItem> ExistingItems = StorageComponent->GetInventory();
	int32 ClearedUnits = 0;
	for (const FInventoryItem& ExistingItem : ExistingItems)
	{
		if (ExistingItem.Count <= 0)
		{
			continue;
		}

		StorageComponent->RemoveItem(ExistingItem, ExistingItem.Count);
		ClearedUnits += ExistingItem.Count;
	}

	if (ClearedUnits > 0)
	{
		UE_LOG(LogPangeaMiningSiteActor, Log, TEXT("Cleared inherited mining chest contents. Actor=%s Chest=%s ClearedUnits=%d"),
			*GetNameSafe(this),
			*GetNameSafe(SpawnedSiteChest),
			ClearedUnits);
	}
}

void AMiningSiteActor::RefreshPresentationActors()
{
	if (IMiningSitePresentationCoordinatorInterface* Coordinator = Cast<IMiningSitePresentationCoordinatorInterface>(PresentationCoordinator))
	{
		Coordinator->RefreshPresentationActors(*this, MiningSiteComponent, SpawnedSiteChest, SpawnedWorkerActors, SpawnedGuardActors, SpawnedCourierActor);
	}
}

void AMiningSiteActor::ClearPresentationActors()
{
	if (IMiningSitePresentationCoordinatorInterface* Coordinator = Cast<IMiningSitePresentationCoordinatorInterface>(PresentationCoordinator))
	{
		Coordinator->ClearPresentationActors(SpawnedWorkerActors, SpawnedGuardActors, SpawnedCourierActor);
	}
}

void AMiningSiteActor::UpdatePresentationActorMovement()
{
	if (IMiningSitePresentationCoordinatorInterface* Coordinator = Cast<IMiningSitePresentationCoordinatorInterface>(PresentationCoordinator))
	{
		Coordinator->UpdatePresentationActorMovement(*this, MiningSiteComponent, SpawnedSiteChest, SpawnedWorkerActors, SpawnedGuardActors, SpawnedCourierActor, WorkerSmartObjectComponents, GuardSmartObjectComponents);
	}
}

void AMiningSiteActor::ConfigureSmartObjectComponents()
{
	if (IMiningSitePresentationCoordinatorInterface* Coordinator = Cast<IMiningSitePresentationCoordinatorInterface>(PresentationCoordinator))
	{
		Coordinator->ConfigureSmartObjectComponents(*this, MiningSiteComponent, WorkerSmartObjectComponents, GuardSmartObjectComponents);
	}
}

void AMiningSiteActor::EnsurePresentationCoordinator()
{
	if (PresentationCoordinator)
	{
		return;
	}

	UClass* CoordinatorClass = LoadClass<UActorComponent>(nullptr, TEXT("/Script/MiningSystemPresentation.MiningSitePresentationCoordinatorComponent"));
	if (!CoordinatorClass)
	{
		UE_LOG(LogPangeaMiningSiteActor, Warning, TEXT("Failed to load presentation coordinator class. Actor=%s"), *GetNameSafe(this));
		return;
	}

	PresentationCoordinator = NewObject<UActorComponent>(this, CoordinatorClass, TEXT("PresentationCoordinator"));
	if (!PresentationCoordinator)
	{
		return;
	}

	AddInstanceComponent(PresentationCoordinator);
	AddOwnedComponent(PresentationCoordinator);
	PresentationCoordinator->RegisterComponent();
}

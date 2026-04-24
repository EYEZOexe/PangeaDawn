#include "Actors/MiningDiscoveryNodeActor.h"

#include "Actors/MiningSiteActor.h"
#include "Components/ACFInteractionComponent.h"
#include "Components/SphereComponent.h"
#include "Components/MiningSiteComponent.h"
#include "Components/StaticMeshComponent.h"
#include "DataAssets/MiningSiteDefinition.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Net/UnrealNetwork.h"

DEFINE_LOG_CATEGORY_STATIC(LogPangeaMiningDiscovery, Log, All);

AMiningDiscoveryNodeActor::AMiningDiscoveryNodeActor()
{
	bReplicates = true;
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	NodeMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("NodeMesh"));
	NodeMesh->SetupAttachment(SceneRoot);
	NodeMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

	InteractionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionSphere"));
	InteractionSphere->SetupAttachment(SceneRoot);
	InteractionSphere->SetSphereRadius(250.0f);
	InteractionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	InteractionSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	InteractionSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	SiteSpawnTransform = FTransform::Identity;
	InteractionText = FText::FromString(TEXT("Establish Mining Site"));
}

void AMiningDiscoveryNodeActor::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogPangeaMiningDiscovery, Log, TEXT("Mining discovery node BeginPlay. Node=%s HasAuthority=%s EstablishedSite=%s Definition=%s SiteClass=%s Settlement=%s Location=%s"),
		*GetNameSafe(this),
		HasAuthority() ? TEXT("true") : TEXT("false"),
		*GetNameSafe(EstablishedSite),
		*GetNameSafe(SiteDefinition),
		*GetNameSafe(MiningSiteActorClass.Get()),
		*GetNameSafe(SettlementResourceActor),
		*GetActorLocation().ToString());

	InteractionSphere->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::HandleInteractionBegin);
	InteractionSphere->OnComponentEndOverlap.AddDynamic(this, &ThisClass::HandleInteractionEnd);
	RefreshLocalInteractionRegistration();
}

void AMiningDiscoveryNodeActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
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

bool AMiningDiscoveryNodeActor::CanSetUpSite() const
{
	return !EstablishedSite && SiteDefinition && MiningSiteActorClass;
}

AMiningSiteActor* AMiningDiscoveryNodeActor::SetUpSite()
{
	if (EstablishedSite)
	{
		UE_LOG(LogPangeaMiningDiscovery, Log, TEXT("SetUpSite skipped. Node=%s already has EstablishedSite=%s"),
			*GetNameSafe(this),
			*GetNameSafe(EstablishedSite));
		return EstablishedSite;
	}

	if (!HasAuthority() || !CanSetUpSite())
	{
		UE_LOG(LogPangeaMiningDiscovery, Warning, TEXT("SetUpSite blocked. Node=%s HasAuthority=%s EstablishedSite=%s Definition=%s SiteClass=%s"),
			*GetNameSafe(this),
			HasAuthority() ? TEXT("true") : TEXT("false"),
			*GetNameSafe(EstablishedSite),
			*GetNameSafe(SiteDefinition),
			*GetNameSafe(MiningSiteActorClass.Get()));
		return nullptr;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogPangeaMiningDiscovery, Warning, TEXT("SetUpSite failed on %s: no world."), *GetNameSafe(this));
		return nullptr;
	}

	const FTransform WorldTransform = SiteSpawnTransform * GetActorTransform();
	AMiningSiteActor* NewSite = World->SpawnActorDeferred<AMiningSiteActor>(MiningSiteActorClass, WorldTransform, this);
	if (!NewSite)
	{
		UE_LOG(LogPangeaMiningDiscovery, Warning, TEXT("SetUpSite failed on %s: SpawnActorDeferred returned null."), *GetNameSafe(this));
		return nullptr;
	}

	if (NewSite->MiningSiteComponent)
	{
		NewSite->MiningSiteComponent->SiteDefinition = SiteDefinition;
	}
	NewSite->SettlementResourceActor = SettlementResourceActor;

	NewSite->FinishSpawning(WorldTransform);

	if (NewSite->MiningSiteComponent && NewSite->MiningSiteComponent->EstablishSite())
	{
		EstablishedSite = NewSite;
		for (const TWeakObjectPtr<UACFInteractionComponent>& InteractionComponent : RegisteredInteractionComponents)
		{
			if (InteractionComponent.IsValid())
			{
				InteractionComponent->UnregisterInteractable(this);
				InteractionComponent->RefreshInteractions();
			}
		}

		UE_LOG(LogPangeaMiningDiscovery, Log, TEXT("SetUpSite succeeded. Node=%s Site=%s Definition=%s Settlement=%s SiteSettlement=%s Location=%s"),
			*GetNameSafe(this),
			*GetNameSafe(EstablishedSite),
			*GetNameSafe(SiteDefinition),
			*GetNameSafe(SettlementResourceActor),
			*GetNameSafe(NewSite->SettlementResourceActor),
			*EstablishedSite->GetActorLocation().ToString());
		return EstablishedSite;
	}

	UE_LOG(LogPangeaMiningDiscovery, Warning, TEXT("SetUpSite spawned %s but EstablishSite failed. Destroying spawned site."),
		*GetNameSafe(NewSite));
	NewSite->Destroy();
	return nullptr;
}

void AMiningDiscoveryNodeActor::OnInteractedByPawn_Implementation(APawn* Pawn, const FString& interactionType)
{
	AMiningSiteActor* NewSite = SetUpSite();
	UE_LOG(LogPangeaMiningDiscovery, Log, TEXT("Discovery interaction resolved. Node=%s Pawn=%s Result=%s Site=%s"),
		*GetNameSafe(this),
		*GetNameSafe(Pawn),
		NewSite ? TEXT("true") : TEXT("false"),
		*GetNameSafe(NewSite));
}

void AMiningDiscoveryNodeActor::OnLocalInteractedByPawn_Implementation(APawn* Pawn, const FString& interactionType)
{
	UE_LOG(LogPangeaMiningDiscovery, Log, TEXT("Discovery locally interacted. Node=%s Pawn=%s"),
		*GetNameSafe(this),
		*GetNameSafe(Pawn));
}

void AMiningDiscoveryNodeActor::OnInteractableRegisteredByPawn_Implementation(APawn* Pawn)
{
	UE_LOG(LogPangeaMiningDiscovery, Log, TEXT("Discovery interactable registered. Node=%s Pawn=%s"),
		*GetNameSafe(this),
		*GetNameSafe(Pawn));
}

void AMiningDiscoveryNodeActor::OnInteractableUnregisteredByPawn_Implementation(APawn* Pawn)
{
	UE_LOG(LogPangeaMiningDiscovery, Log, TEXT("Discovery interactable unregistered. Node=%s Pawn=%s"),
		*GetNameSafe(this),
		*GetNameSafe(Pawn));
}

FText AMiningDiscoveryNodeActor::GetInteractableName_Implementation()
{
	return CanSetUpSite() ? InteractionText : FText::GetEmpty();
}

bool AMiningDiscoveryNodeActor::CanBeInteracted_Implementation(APawn* Pawn)
{
	return Pawn != nullptr && CanSetUpSite();
}

void AMiningDiscoveryNodeActor::RefreshLocalInteractionRegistration()
{
	if (!InteractionSphere)
	{
		return;
	}

	TArray<AActor*> OverlappingActors;
	InteractionSphere->GetOverlappingActors(OverlappingActors, APawn::StaticClass());
	for (AActor* OverlappingActor : OverlappingActors)
	{
		if (APawn* OverlappingPawn = Cast<APawn>(OverlappingActor))
		{
			if (UACFInteractionComponent* InteractionComponent = OverlappingPawn->FindComponentByClass<UACFInteractionComponent>())
			{
				RegisterWithInteractionComponent(InteractionComponent);
			}
		}
	}
}

void AMiningDiscoveryNodeActor::RegisterWithInteractionComponent(UACFInteractionComponent* InteractionComponent)
{
	if (!InteractionComponent)
	{
		return;
	}

	InteractionComponent->RegisterInteractable(this);
	RegisteredInteractionComponents.AddUnique(InteractionComponent);
}

void AMiningDiscoveryNodeActor::UnregisterFromInteractionComponent(UACFInteractionComponent* InteractionComponent)
{
	if (!InteractionComponent)
	{
		return;
	}

	InteractionComponent->UnregisterInteractable(this);
	RegisteredInteractionComponents.RemoveAll([InteractionComponent](const TWeakObjectPtr<UACFInteractionComponent>& Entry)
	{
		return Entry.Get() == InteractionComponent;
	});
}

void AMiningDiscoveryNodeActor::HandleInteractionBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (APawn* OverlappingPawn = Cast<APawn>(OtherActor))
	{
		RegisterWithInteractionComponent(OverlappingPawn->FindComponentByClass<UACFInteractionComponent>());
		UE_LOG(LogPangeaMiningDiscovery, Log, TEXT("Mining discovery overlap begin. Node=%s Interactor=%s"), *GetNameSafe(this), *GetNameSafe(OverlappingPawn));
	}
}

void AMiningDiscoveryNodeActor::HandleInteractionEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (APawn* OverlappingPawn = Cast<APawn>(OtherActor))
	{
		UnregisterFromInteractionComponent(OverlappingPawn->FindComponentByClass<UACFInteractionComponent>());
		UE_LOG(LogPangeaMiningDiscovery, Log, TEXT("Mining discovery overlap end. Node=%s Interactor=%s"), *GetNameSafe(this), *GetNameSafe(OverlappingPawn));
	}
}

void AMiningDiscoveryNodeActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AMiningDiscoveryNodeActor, EstablishedSite);
}

#include "Actions/UA_UnlockMiningSite.h"

#include "Actors/VillageBase.h"
#include "Actors/MiningDiscoveryNodeActor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"
#include "Types/BaseUpgradeContext.h"

void UUA_UnlockMiningSite::ExecuteForContext(const FBaseUpgradeContext& Context)
{
	if (Context.Village)
	{
		ApplyToWorld(Context.Village->GetWorld());
		return;
	}

	if (UObject* ActionObject = Context.GetPreferredActionObject())
	{
		ApplyToWorld(ActionObject->GetWorld());
	}
}

void UUA_UnlockMiningSite::Execute_Implementation(UObject* ContextObject)
{
	if (!ContextObject)
	{
		return;
	}

	ApplyToWorld(ContextObject->GetWorld());
}

void UUA_UnlockMiningSite::ApplyToWorld(UWorld* World) const
{
	if (!World)
	{
		return;
	}

	for (TActorIterator<AMiningDiscoveryNodeActor> It(World); It; ++It)
	{
		ApplyToActor(*It);
	}
}

void UUA_UnlockMiningSite::ApplyToActor(AActor* Actor) const
{
	AMiningDiscoveryNodeActor* DiscoveryNode = Cast<AMiningDiscoveryNodeActor>(Actor);
	if (!DiscoveryNode)
	{
		return;
	}

	if (FacilityTag.IsValid() && DiscoveryNode->FacilityTag != FacilityTag)
	{
		return;
	}

	switch (Mode)
	{
	case EMiningUnlockMode::Unlock:
		DiscoveryNode->SetUnlocked(true);
		break;
	case EMiningUnlockMode::Lock:
		DiscoveryNode->SetUnlocked(false);
		break;
	case EMiningUnlockMode::Toggle:
		DiscoveryNode->SetUnlocked(!DiscoveryNode->bUnlocked);
		break;
	default:
		break;
	}
}

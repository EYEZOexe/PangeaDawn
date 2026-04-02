#include "Types/BaseUpgradeContext.h"

#include "Actors/VillageBase.h"

UObject* FBaseUpgradeContext::GetPreferredRequirementObject() const
{
	if (InteractingPawn)
	{
		return InteractingPawn;
	}

	if (Village)
	{
		return Village;
	}

	if (SourceObject)
	{
		return SourceObject;
	}

	return nullptr;
}

UObject* FBaseUpgradeContext::GetPreferredActionObject() const
{
	if (Village)
	{
		return Village;
	}

	if (InteractingPawn)
	{
		return InteractingPawn;
	}

	if (SourceObject)
	{
		return SourceObject;
	}

	return nullptr;
}

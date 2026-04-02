#include "DataAssets/BaseUpgradeDefinition.h"

#include "Definitions/BaseUpgradeFragment.h"

UBaseUpgradeFragment* UBaseUpgradeDefinition::GetFragmentByClass(TSubclassOf<UBaseUpgradeFragment> FragmentClass) const
{
	if (!FragmentClass)
	{
		return nullptr;
	}

	for (UBaseUpgradeFragment* Fragment : Fragments)
	{
		if (Fragment && Fragment->IsA(FragmentClass))
		{
			return Fragment;
		}
	}

	return nullptr;
}

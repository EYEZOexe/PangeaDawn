#include "Definitions/PangeaCreatureDefinition.h"

#include "Definitions/PangeaDefinitionFragment.h"

UPangeaDefinitionFragment* UPangeaCreatureDefinition::GetFragmentByClass(TSubclassOf<UPangeaDefinitionFragment> FragmentClass) const
{
	if (!FragmentClass)
	{
		return nullptr;
	}

	for (UPangeaDefinitionFragment* Fragment : Fragments)
	{
		if (Fragment && Fragment->IsA(FragmentClass))
		{
			return Fragment;
		}
	}

	return nullptr;
}

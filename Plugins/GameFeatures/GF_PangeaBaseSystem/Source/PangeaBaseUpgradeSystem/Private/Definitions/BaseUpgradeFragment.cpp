#include "Definitions/BaseUpgradeFragment.h"

#include "DataAssets/BaseUpgradeDefinition.h"

UBaseUpgradeDefinition* UBaseUpgradeFragment::GetOwningDefinition() const
{
	return GetTypedOuter<UBaseUpgradeDefinition>();
}

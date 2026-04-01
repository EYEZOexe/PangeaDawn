#include "Definitions/PangeaDefinitionFragment.h"

#include "Definitions/PangeaCreatureDefinition.h"

UPangeaCreatureDefinition* UPangeaDefinitionFragment::GetOwningDefinition() const
{
	return GetTypedOuter<UPangeaCreatureDefinition>();
}

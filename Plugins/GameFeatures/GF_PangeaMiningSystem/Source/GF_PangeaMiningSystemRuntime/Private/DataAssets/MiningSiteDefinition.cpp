#include "DataAssets/MiningSiteDefinition.h"

bool UMiningSiteDefinition::GetLevelDefinition(const int32 Level, FMiningSiteLevelDefinition& OutDefinition) const
{
	for (const FMiningSiteLevelDefinition& Definition : Levels)
	{
		if (Definition.Level == Level)
		{
			OutDefinition = Definition;
			return true;
		}
	}

	return false;
}

int32 UMiningSiteDefinition::GetMaxLevel() const
{
	int32 MaxLevel = INDEX_NONE;
	for (const FMiningSiteLevelDefinition& Definition : Levels)
	{
		MaxLevel = FMath::Max(MaxLevel, Definition.Level);
	}

	return MaxLevel;
}

#if WITH_EDITOR
EDataValidationResult UMiningSiteDefinition::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = EDataValidationResult::Valid;

	if (PresentationConfig.IsNull())
	{
		Context.AddError(FText::FromString(TEXT("PresentationConfig must be assigned.")));
		Result = EDataValidationResult::Invalid;
	}

	if (!PresentationConfig.IsNull() && !PresentationConfig.LoadSynchronous())
	{
		Context.AddError(FText::FromString(TEXT("PresentationConfig could not be loaded.")));
		Result = EDataValidationResult::Invalid;
	}

	return Result;
}
#endif

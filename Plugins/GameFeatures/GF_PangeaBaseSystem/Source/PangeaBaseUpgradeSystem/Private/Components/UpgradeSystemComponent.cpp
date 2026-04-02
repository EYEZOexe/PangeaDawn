#include "Components/UpgradeSystemComponent.h"

#include "Actions/UA_EnableFacility.h"
#include "Actors/VillageBase.h"
#include "Components/FacilityManagerComponent.h"
#include "DataAssets/BaseUpgradeDefinition.h"
#include "Definitions/Fragments/BaseFacilityCatalogFragment.h"
#include "Definitions/Fragments/BaseProgressionFragment.h"
#include "Definitions/Fragments/BaseSaveRulesFragment.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Objects/UpgradeAction.h"
#include "Objects/UpgradeRequirement.h"

UPangeaUpgradeSystemComponent::UPangeaUpgradeSystemComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UPangeaUpgradeSystemComponent::BeginPlay()
{
	Super::BeginPlay();
}

FBaseUpgradeContext UPangeaUpgradeSystemComponent::MakeUpgradeContext(UObject* PlayerContext) const
{
	FBaseUpgradeContext Context;
	Context.SourceObject = PlayerContext ? PlayerContext : GetOwner();
	Context.UpgradeSystem = const_cast<UPangeaUpgradeSystemComponent*>(this);
	Context.FacilityManager = GetOwner() ? GetOwner()->FindComponentByClass<UPangeaFacilityManagerComponent>() : nullptr;
	Context.Village = Cast<AVillageBase>(GetOwner());
	Context.InteractingPawn = Cast<APawn>(PlayerContext);

	if (Context.InteractingPawn)
	{
		Context.PlayerController = Cast<APlayerController>(Context.InteractingPawn->GetController());
	}
	else if (APlayerController* PlayerController = Cast<APlayerController>(PlayerContext))
	{
		Context.PlayerController = PlayerController;
		Context.InteractingPawn = PlayerController->GetPawn();
	}

	return Context;
}

void UPangeaUpgradeSystemComponent::LoadCompletedMilestones(UObject* PlayerContext)
{
	if (!HasDefinitionData())
	{
		UE_LOG(LogTemp, Error, TEXT("[REPLAY] No upgrade definition assigned"));
		return;
	}

	const UBaseSaveRulesFragment* SaveRules = UpgradeDefinition ? UpgradeDefinition->GetFragment<UBaseSaveRulesFragment>() : nullptr;
	if (SaveRules && !SaveRules->bReplayCompletedMilestonesOnLoad)
	{
		return;
	}

	FBaseUpgradeContext UpgradeContext = MakeUpgradeContext(PlayerContext);
	TArray<FGameplayTag> CompletedTags;
	CompletedMilestones.GetGameplayTagArray(CompletedTags);

	TArray<FUpgradeLevelDefinition> AllLevels;
	if (const UBaseProgressionFragment* Progression = GetProgressionFragment())
	{
		for (const FUpgradeLevelDefinition& LevelDef : Progression->Levels)
		{
			AllLevels.Add(LevelDef);
		}
	}

	for (const FGameplayTag& CompletedTag : CompletedTags)
	{
		for (const FUpgradeLevelDefinition& LevelDef : AllLevels)
		{
			for (const FUpgradeMilestoneDefinition& Milestone : LevelDef.Milestones)
			{
				if (Milestone.MilestoneTag != CompletedTag)
				{
					continue;
				}

				for (UUpgradeAction* Action : Milestone.Actions)
				{
					if (Action)
					{
						Action->ExecuteForContext(UpgradeContext);
					}
				}
			}
		}
	}
}

bool UPangeaUpgradeSystemComponent::LoadCompletedMilestonesForContext_Implementation(UObject* PlayerContext)
{
	LoadCompletedMilestones(PlayerContext);
	return HasDefinitionData();
}

void UPangeaUpgradeSystemComponent::OnLevelIncreased(int32 NewLevel, UObject* PlayerContext)
{
	if (!HasDefinitionData())
	{
		UE_LOG(LogTemp, Error, TEXT("UpgradeSystem: OnLevelIncreased called but no definition is assigned on %s"),
			*GetNameSafe(GetOwner()));
		return;
	}

	if (NewLevel <= CurrentLevel)
	{
		return;
	}

	CurrentLevel = NewLevel;
	ExecuteMilestonesForLevel(CurrentLevel, MakeUpgradeContext(PlayerContext));
}

bool UPangeaUpgradeSystemComponent::CanUpgradeToNextLevel(UObject* PlayerContext) const
{
	if (!HasDefinitionData())
	{
		return false;
	}

	const int32 TargetLevel = CurrentLevel + 1;
	FUpgradeLevelDefinition LevelDef;
	if (!FindLevelDefinition(TargetLevel, LevelDef))
	{
		return false;
	}

	const FBaseUpgradeContext UpgradeContext = MakeUpgradeContext(PlayerContext);

	for (const FUpgradeMilestoneDefinition& Milestone : LevelDef.Milestones)
	{
		for (UUpgradeRequirement* Requirement : Milestone.Requirements)
		{
			if (Requirement && !Requirement->IsRequirementMetForContext(UpgradeContext))
			{
				return false;
			}
		}
	}

	return true;
}

bool UPangeaUpgradeSystemComponent::CanUpgradeToNextLevelForContext_Implementation(UObject* PlayerContext) const
{
	return CanUpgradeToNextLevel(PlayerContext);
}

bool UPangeaUpgradeSystemComponent::IsMilestoneCompleted(FGameplayTag MilestoneTag) const
{
	return CompletedMilestones.HasTag(MilestoneTag);
}

void UPangeaUpgradeSystemComponent::MarkMilestoneCompleted(FGameplayTag MilestoneTag)
{
	if (!CompletedMilestones.HasTag(MilestoneTag))
	{
		CompletedMilestones.AddTag(MilestoneTag);
	}
}

bool UPangeaUpgradeSystemComponent::TryUpgradeToNextLevel_Implementation(UObject* PlayerContext)
{
	if (!CanUpgradeToNextLevel(PlayerContext))
	{
		return false;
	}

	OnLevelIncreased(CurrentLevel + 1, PlayerContext);
	return true;
}

int32 UPangeaUpgradeSystemComponent::GetCurrentUpgradeLevel_Implementation() const
{
	return CurrentLevel;
}

bool UPangeaUpgradeSystemComponent::IsUpgradeMilestoneCompleted_Implementation(FGameplayTag MilestoneTag) const
{
	return IsMilestoneCompleted(MilestoneTag);
}

bool UPangeaUpgradeSystemComponent::GetNextLevelDefinition(FUpgradeLevelDefinition& OutLevel) const
{
	return FindLevelDefinition(CurrentLevel + 1, OutLevel);
}

bool UPangeaUpgradeSystemComponent::GetNextUpgradeLevelDefinition_Implementation(FUpgradeLevelDefinition& OutLevel) const
{
	return GetNextLevelDefinition(OutLevel);
}

void UPangeaUpgradeSystemComponent::GetMilestonesForLevel(int32 Level, TArray<FUpgradeMilestoneDefinition>& OutMilestones) const
{
	OutMilestones.Empty();

	FUpgradeLevelDefinition LevelDef;
	if (FindLevelDefinition(Level, LevelDef))
	{
		OutMilestones = MoveTemp(LevelDef.Milestones);
	}
}

void UPangeaUpgradeSystemComponent::GetUnmetRequirementsForNextLevel(UObject* PlayerContext, TArray<UUpgradeRequirement*>& OutRequirements) const
{
	OutRequirements.Empty();

	FUpgradeLevelDefinition LevelDef;
	if (!FindLevelDefinition(CurrentLevel + 1, LevelDef))
	{
		return;
	}

	const FBaseUpgradeContext UpgradeContext = MakeUpgradeContext(PlayerContext);

	for (const FUpgradeMilestoneDefinition& Milestone : LevelDef.Milestones)
	{
		for (UUpgradeRequirement* Requirement : Milestone.Requirements)
		{
			if (Requirement && !Requirement->IsRequirementMetForContext(UpgradeContext))
			{
				OutRequirements.Add(Requirement);
			}
		}
	}
}

void UPangeaUpgradeSystemComponent::GetUnmetRequirementsForNextUpgrade_Implementation(UObject* PlayerContext, TArray<UUpgradeRequirement*>& OutRequirements) const
{
	GetUnmetRequirementsForNextLevel(PlayerContext, OutRequirements);
}

void UPangeaUpgradeSystemComponent::GetFacilitiesUnlockedAtLevel(int32 Level, TArray<FGameplayTag>& OutFacilities) const
{
	OutFacilities.Empty();

	FUpgradeLevelDefinition LevelDef;
	if (!FindLevelDefinition(Level, LevelDef))
	{
		return;
	}

	for (const FUpgradeMilestoneDefinition& Milestone : LevelDef.Milestones)
	{
		for (UUpgradeAction* Action : Milestone.Actions)
		{
			if (const UUA_EnableFacility* EnableAction = Cast<UUA_EnableFacility>(Action))
			{
				if (EnableAction->FacilityTag.IsValid())
				{
					OutFacilities.AddUnique(EnableAction->FacilityTag);
				}
			}
		}
	}
}

void UPangeaUpgradeSystemComponent::GetFacilitiesUnlockedAtUpgradeLevel_Implementation(int32 Level, TArray<FGameplayTag>& OutFacilities) const
{
	GetFacilitiesUnlockedAtLevel(Level, OutFacilities);
}

FText UPangeaUpgradeSystemComponent::GetFacilityDisplayName(const FGameplayTag& FacilityTag) const
{
	if (const UBaseFacilityCatalogFragment* Catalog = GetFacilityCatalogFragment())
	{
		for (const FFacilityGroupReference& FacilityGroup : Catalog->FacilityGroups)
		{
			if (FacilityGroup.FacilityTag == FacilityTag)
			{
				return FacilityGroup.FacilityGroupName;
			}
		}
	}

	return FText::FromString(FacilityTag.ToString());
}

FText UPangeaUpgradeSystemComponent::GetUpgradeFacilityDisplayName_Implementation(FGameplayTag FacilityTag) const
{
	return GetFacilityDisplayName(FacilityTag);
}

bool UPangeaUpgradeSystemComponent::HasDefinitionData() const
{
	return UpgradeDefinition != nullptr;
}

const UBaseProgressionFragment* UPangeaUpgradeSystemComponent::GetProgressionFragment() const
{
	return UpgradeDefinition ? UpgradeDefinition->GetFragment<UBaseProgressionFragment>() : nullptr;
}

const UBaseFacilityCatalogFragment* UPangeaUpgradeSystemComponent::GetFacilityCatalogFragment() const
{
	return UpgradeDefinition ? UpgradeDefinition->GetFragment<UBaseFacilityCatalogFragment>() : nullptr;
}

bool UPangeaUpgradeSystemComponent::ResolveLevelDefinition(int32 Level, FUpgradeLevelDefinition& OutLevel) const
{
	if (const UBaseProgressionFragment* Progression = GetProgressionFragment())
	{
		for (const FUpgradeLevelDefinition& LevelDef : Progression->Levels)
		{
			if (LevelDef.Level == Level)
			{
				OutLevel = LevelDef;
				return true;
			}
		}
	}

	return false;
}

bool UPangeaUpgradeSystemComponent::FindLevelDefinition(int32 Level, FUpgradeLevelDefinition& OutLevel) const
{
	return ResolveLevelDefinition(Level, OutLevel);
}

void UPangeaUpgradeSystemComponent::ExecuteMilestonesForLevel(int32 Level, const FBaseUpgradeContext& UpgradeContext)
{
	FUpgradeLevelDefinition LevelDef;
	if (!FindLevelDefinition(Level, LevelDef))
	{
		return;
	}

	for (const FUpgradeMilestoneDefinition& Milestone : LevelDef.Milestones)
	{
		if (Milestone.MilestoneTag.IsValid() && IsMilestoneCompleted(Milestone.MilestoneTag))
		{
			continue;
		}

		bool bAllRequirementsMet = true;
		for (UUpgradeRequirement* Requirement : Milestone.Requirements)
		{
			if (Requirement && !Requirement->IsRequirementMetForContext(UpgradeContext))
			{
				bAllRequirementsMet = false;
				break;
			}
		}

		if (!bAllRequirementsMet)
		{
			continue;
		}

		for (UUpgradeAction* Action : Milestone.Actions)
		{
			if (Action)
			{
				Action->ExecuteForContext(UpgradeContext);
			}
		}

		if (Milestone.MilestoneTag.IsValid())
		{
			MarkMilestoneCompleted(Milestone.MilestoneTag);
		}
	}
}

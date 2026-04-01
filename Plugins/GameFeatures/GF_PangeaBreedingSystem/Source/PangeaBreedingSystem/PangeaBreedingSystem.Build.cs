// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class PangeaBreedingSystem : ModuleRules
{
	public PangeaBreedingSystem(ReadOnlyTargetRules Target) : base(Target)
	{
		PrivateDependencyModuleNames.AddRange(new string[] { "StatusEffectSystem", "ActionsSystem", "AdvancedRPGSystem", "GameplayAbilities" });
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "InventorySystem", "UMG", "PangeaCore", "GameplayTags" });
	}
}

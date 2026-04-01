// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class PangeaDinosaurAI : ModuleRules
{
	public PangeaDinosaurAI(ReadOnlyTargetRules Target) : base(Target)
	{
		PrivateDependencyModuleNames.AddRange(new string[] { "MountSystem", "AscentSaveSystem", "AscentCoreInterfaces", "AscentCombatFramework", "CharacterController", });
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "GameplayTags", "PangeaCore", "ModularGameplay" });
	}
}

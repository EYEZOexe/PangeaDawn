// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class GF_PangeaMiningSystemRuntime : ModuleRules
{
	public GF_PangeaMiningSystemRuntime(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicIncludePaths.AddRange(
			new string[] {
				// ... add public include paths required here ...
			}
			);
				
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"GameplayTags",
				"SmartObjectsModule",
				"PangeaCore",
				"AscentCombatFramework",
				"AscentCoreInterfaces",
				"InventorySystem",
				"UMG",
			}
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"AIModule",
				"NavigationSystem",
			}
			);
		
		
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
			);
	}
}

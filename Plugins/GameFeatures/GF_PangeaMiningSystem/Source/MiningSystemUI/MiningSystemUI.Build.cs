using UnrealBuildTool;

public class MiningSystemUI : ModuleRules
{
    public MiningSystemUI(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "CoreUObject",
                "Engine",
                "InventorySystem",
                "AscentCombatFramework",
                "GF_PangeaMiningSystemRuntime",
                "UMG",
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "Slate",
                "SlateCore",
            }
        );

    }
}

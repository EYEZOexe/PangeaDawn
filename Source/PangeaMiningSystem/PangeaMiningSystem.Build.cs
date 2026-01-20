using UnrealBuildTool;

public class PangeaMiningSystem : ModuleRules
{
    public PangeaMiningSystem(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "InventorySystem",
                "AIFramework",
                "AscentCombatFramework",
                "AscentCoreInterfaces"
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "CoreUObject",
                "Engine",
                "Slate",
                "SlateCore"
            }
        );
    }
}
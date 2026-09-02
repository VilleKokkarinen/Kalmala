using UnrealBuildTool;

public class KalmalaEditor : ModuleRules
{
    public KalmalaEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PrivateDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "UnrealEd",
            "KalmalaGameplay",
            "KalmalaWorld"
        });
    }
}

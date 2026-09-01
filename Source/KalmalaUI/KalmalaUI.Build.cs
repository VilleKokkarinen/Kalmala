using UnrealBuildTool;

public class KalmalaUI : ModuleRules
{
    public KalmalaUI(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "UMG",
            "CommonUI",
            "KalmalaCore"
        });
    }
}

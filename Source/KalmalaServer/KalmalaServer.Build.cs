using UnrealBuildTool;

public class KalmalaServer : ModuleRules
{
    public KalmalaServer(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "NetCore",
            "KalmalaCore",
            "KalmalaGameplay",
            "KalmalaWorld"
        });
    }
}

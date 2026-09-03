using UnrealBuildTool;

public class KalmalaWorld : ModuleRules
{
    public KalmalaWorld(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "GameplayTags",
            "KalmalaCore",
            "ProceduralMeshComponent"
        });
    }
}

using UnrealBuildTool;

public class KalmalaGameplay : ModuleRules
{
    public KalmalaGameplay(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "NetCore",
            "OnlineSubsystem",
            "OnlineSubsystemUtils",
            "EnhancedInput",
            "GameplayAbilities",
            "GameplayTags",
            "GameplayTasks",
            "KalmalaCore",
            "KalmalaWorld",
            "ProceduralMeshComponent"
        });
    }
}

using UnrealBuildTool;
using System.Collections.Generic;

public class KalmalaServerTarget : TargetRules
{
    public KalmalaServerTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Server;
        DefaultBuildSettings = BuildSettingsVersion.V7;
        IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_8;
        ExtraModuleNames.AddRange(new string[] { "KalmalaCore", "KalmalaGameplay", "KalmalaWorld", "KalmalaServer" });
    }
}

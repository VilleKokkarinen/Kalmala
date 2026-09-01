using UnrealBuildTool;
using System.Collections.Generic;

public class KalmalaTarget : TargetRules
{
    public KalmalaTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Game;
        DefaultBuildSettings = BuildSettingsVersion.V7;
        IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_8;
        ExtraModuleNames.AddRange(new string[] { "KalmalaCore", "KalmalaGameplay", "KalmalaWorld", "KalmalaUI" });
    }
}

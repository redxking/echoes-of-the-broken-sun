using UnrealBuildTool;
using System.Collections.Generic;

public class EchoesOfTheBrokenSunTarget : TargetRules
{
    public EchoesOfTheBrokenSunTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Game;
        DefaultBuildSettings = BuildSettingsVersion.V5;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        ExtraModuleNames.AddRange(new[] { "EchoesSimCore", "EchoesOfTheBrokenSun" });
    }
}


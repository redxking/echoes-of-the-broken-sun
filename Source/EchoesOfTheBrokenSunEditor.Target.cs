using UnrealBuildTool;
using System.Collections.Generic;

public class EchoesOfTheBrokenSunEditorTarget : TargetRules
{
    public EchoesOfTheBrokenSunEditorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;
        DefaultBuildSettings = BuildSettingsVersion.V7;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        ExtraModuleNames.AddRange(new[] { "EchoesSimCore", "EchoesOfTheBrokenSun" });
    }
}

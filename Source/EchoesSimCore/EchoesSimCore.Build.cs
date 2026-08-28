using UnrealBuildTool;

public class EchoesSimCore : ModuleRules
{
    public EchoesSimCore(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.NoPCHs;
        bUseUnity = false;
        CppStandard = CppStandardVersion.Cpp20;

        // The C++ source intentionally has no Unreal dependencies. An Unreal-facing
        // adapter can depend on this module without pulling engine types into the sim.
    }
}

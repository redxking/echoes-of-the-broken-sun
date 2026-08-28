using UnrealBuildTool;

public class EchoesSimCore : ModuleRules
{
    public EchoesSimCore(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.NoPCHs;
        bUseUnity = false;
        CppStandard = CppStandardVersion.Cpp20;

        PublicDependencyModuleNames.Add("Core");

        // The algorithms and data structures remain standard C++20. Core supplies
        // Unreal's module export/platform macros when this code is built as a module;
        // the same source is compiled directly by the native test harness.
    }
}

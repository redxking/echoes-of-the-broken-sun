using UnrealBuildTool;

public class EchoesOfTheBrokenSun : ModuleRules
{
    public EchoesOfTheBrokenSun(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        CppStandard = CppStandardVersion.Cpp20;
        PublicDependencyModuleNames.AddRange(new[]
        {
            "Core", "CoreUObject", "Engine", "InputCore", "ApplicationCore", "PhysicsCore",
            "AIModule", "NavigationSystem", "GameplayTasks", "UMG", "Slate",
            "SlateCore", "Json", "JsonUtilities", "Sockets", "EchoesSimCore"
        });
    }
}

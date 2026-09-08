using UnrealBuildTool;

public class RiftNiagaraAuthoring : ModuleRules
{
    public RiftNiagaraAuthoring(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine", "Niagara" });
        PrivateDependencyModuleNames.AddRange(new[] { "UnrealEd", "NiagaraEditor", "AssetTools", "Json", "JsonUtilities", "RenderCore" });
    }
}

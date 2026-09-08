#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "RiftNiagaraAuthoringLibrary.generated.h"

/**
 * Editor-only, sandbox authoring endpoint. Python may call this reflected function as
 * ``unreal.RiftNiagaraAuthoringLibrary.author_shard(package_path, material_path)``.
 * It creates a Niagara asset only at the supplied package path; it performs no game binding.
 */
UCLASS()
class RIFTNIAGARAAUTHORING_API URiftNiagaraAuthoringLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category="Echoes|Sandbox Niagara")
    static FString AuthorShard(const FString& PackagePath, const FString& MaterialPath);

    UFUNCTION(BlueprintCallable, Category="Echoes|Sandbox Niagara")
    static FString ReadParticles(class UNiagaraComponent* Component);

    UFUNCTION(BlueprintCallable, Category="Echoes|Sandbox Niagara")
    static void FinishPreviewCompilation();
};

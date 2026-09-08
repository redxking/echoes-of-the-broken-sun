#include "RiftNiagaraAuthoringLibrary.h"

#include "Editor.h"
#include "Subsystems/EditorAssetSubsystem.h"
#include "Misc/PackageName.h"
#include "Materials/MaterialInterface.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

#include "NiagaraExternalSystemEditorUtilities.h"
#include "NiagaraEmitter.h"
#include "NiagaraEmitterFactoryNew.h"
#include "NiagaraEmitterHandle.h"
#include "NiagaraRendererProperties.h"
#include "NiagaraScript.h"
#include "NiagaraSpriteRendererProperties.h"
#include "NiagaraSystem.h"
#include "NiagaraTypes.h"

namespace RiftNiagaraAuthoring
{
    static const TCHAR* MinimalEmitterPath = TEXT("/Niagara/DefaultAssets/Templates/Emitters/Minimal.Minimal");
    static const TCHAR* SpawnBurstPath = TEXT("/Niagara/Modules/Emitter/SpawnBurst_Instantaneous.SpawnBurst_Instantaneous");
    static const TCHAR* AddVelocityPath = TEXT("/Niagara/Modules/Spawn/Velocity/AddVelocity.AddVelocity");

    static FString WriteReceipt(bool bSuccess, const FString& PackagePath, const FString& MaterialPath, const TArray<FString>& Errors, const TMap<FString, FString>& Facts)
    {
        TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
        Root->SetBoolField(TEXT("success"), bSuccess);
        Root->SetStringField(TEXT("package_path"), PackagePath);
        Root->SetStringField(TEXT("material_path"), MaterialPath);
        Root->SetStringField(TEXT("scope"), TEXT("SANDBOX_ONLY_NO_GAME_BINDING"));
        Root->SetStringField(TEXT("author"), TEXT("Angelis Pseftis"));
        for (const TPair<FString, FString>& Fact : Facts)
        {
            Root->SetStringField(Fact.Key, Fact.Value);
        }
        TArray<TSharedPtr<FJsonValue>> ErrorValues;
        for (const FString& Error : Errors)
        {
            ErrorValues.Add(MakeShared<FJsonValueString>(Error));
        }
        Root->SetArrayField(TEXT("errors"), ErrorValues);
        FString Json;
        const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Json);
        FJsonSerializer::Serialize(Root, Writer);
        return Json;
    }

    static TArray<FString> ContextErrors(const FNiagaraExternalEditContext& Context)
    {
        TArray<FString> Result;
        for (const FText& Error : Context.Errors)
        {
            Result.Add(Error.ToString());
        }
        return Result;
    }

    static TArray<FString> OneError(const FString& Error)
    {
        TArray<FString> Result;
        Result.Add(Error);
        return Result;
    }

    static const FNiagaraExt_ModuleTopology* FindModule(const FNiagaraExt_ScriptStackTopology& Stack, const FString& Token)
    {
        for (const FNiagaraExt_ModuleTopology& Module : Stack.Modules)
        {
            if (Module.ModuleName.ToString().Contains(Token, ESearchCase::IgnoreCase))
            {
                return &Module;
            }
        }
        return nullptr;
    }

    static const FNiagaraExt_StackInputTopology* FindInput(const FNiagaraExt_ModuleTopology& Module, std::initializer_list<const TCHAR*> Candidates)
    {
        for (const TCHAR* Candidate : Candidates)
        {
            for (const FNiagaraExt_StackInputTopology& Input : Module.Inputs)
            {
                if (Input.Name.ToString().Equals(Candidate, ESearchCase::IgnoreCase))
                {
                    return &Input;
                }
            }
        }
        return nullptr;
    }

    static bool SetFloatInput(UNiagaraSystem* System, FName EmitterName, FName ScriptName, const FNiagaraExt_ModuleTopology& Module,
        std::initializer_list<const TCHAR*> Candidates, float Number, FNiagaraExternalEditContext& Context)
    {
        const FNiagaraExt_StackInputTopology* Input = FindInput(Module, Candidates);
        if (!Input)
        {
            FString Available;
            for (const auto& Entry : Module.Inputs) { Available += Entry.Name.ToString() + TEXT(", "); }
            Context.Error(FText::FromString(FString::Printf(TEXT("Required numeric input absent on '%s'. Available: %s"), *Module.ModuleName.ToString(), *Available)));
            return false;
        }
        FNiagaraExt_StackItemReference InputRef(System, EmitterName, ScriptName, Module.ModuleName);
        InputRef.InputNameStack.Reset();
        InputRef.InputNameStack.Add(Input->Name);
        FNiagaraExt_StackInputValue Value;
        if (Input->Type == FNiagaraTypeDefinition::GetIntDef())
        {
            Value.InitializeAs<FNiagaraInt32>().Value = static_cast<int32>(Number);
        }
        else if (Input->Type == FNiagaraTypeDefinition::GetFloatDef())
        {
            Value.InitializeAs<FNiagaraFloat>().Value = Number;
        }
        else
        {
            Context.Error(FText::FromString(TEXT("Required numeric input has incompatible type.")));
            return false;
        }
        UNiagaraExternalEditUtilities::SetStackInputData(InputRef, Value, Context);
        return !Context.HasErrors();
    }

    static bool SetVelocityInput(UNiagaraSystem* System, FName EmitterName, FName ScriptName, const FNiagaraExt_ModuleTopology& Module,
        FNiagaraExternalEditContext& Context)
    {
        const FNiagaraExt_StackInputTopology* Input = FindInput(Module, {TEXT("Velocity")});
        if (!Input)
        {
            Context.Error(FText::FromString(FString::Printf(TEXT("Required Velocity input is absent on module '%s'."), *Module.ModuleName.ToString())));
            return false;
        }
        FNiagaraExt_StackItemReference InputRef(System, EmitterName, ScriptName, Module.ModuleName);
        InputRef.InputNameStack.Reset();
        InputRef.InputNameStack.Add(Input->Name);
        FNiagaraExt_StackInputValue Value;
        FNiagaraVariable Variable(FNiagaraTypeDefinition::GetVec3Def(), TEXT("User.ShardVelocity"));
        System->GetExposedParameters().SetParameterValue<FVector3f>(FVector3f(1200,0,0),Variable,true);
        auto& Linked=Value.InitializeAs<FNiagaraExt_StackInputData_Linked>();
        Linked.LinkedVariable.Name=Variable.GetName(); Linked.LinkedVariable.Type=Variable.GetType();
        UNiagaraExternalEditUtilities::SetStackInputData(InputRef, Value, Context);
        return !Context.HasErrors();
    }

    static bool RefreshTopology(UNiagaraSystem* System, FName EmitterName, FNiagaraExt_EmitterTopology& OutTopology, FNiagaraExternalEditContext& Context)
    {
        FNiagaraExt_StackItemReference EmitterRef(System);
        EmitterRef.EmitterName = EmitterName;
        UNiagaraExternalEditUtilities::GetEmitterTopology(EmitterRef, OutTopology, Context);
        return !Context.HasErrors();
    }
}

FString URiftNiagaraAuthoringLibrary::AuthorShard(const FString& PackagePath, const FString& MaterialPath)
{
    using namespace RiftNiagaraAuthoring;
    TMap<FString, FString> Facts;
    Facts.Add(TEXT("template_emitter"), TEXT("Fresh factory emitter with explicit default modules; no stock-template assumption"));
    Facts.Add(TEXT("spawn_module"), TEXT("SpawnBurst_Instantaneous (one shard)"));
    Facts.Add(TEXT("velocity_cm_s"), TEXT("+X 1200"));
    Facts.Add(TEXT("lifetime_s"), TEXT("0.5"));
    Facts.Add(TEXT("fixed_bounds_cm"), TEXT("min(-620,-620,-620), max(620,620,620)"));
    Facts.Add(TEXT("renderer_shadows"), TEXT("disabled"));

    if (!PackagePath.StartsWith(TEXT("/Game/")) || !FPackageName::IsValidLongPackageName(PackagePath, false))
    {
        return WriteReceipt(false, PackagePath, MaterialPath, OneError(TEXT("package_path must be a new long package name below /Game, for example /Game/ArtSandbox/NS_EBS_RiftShard.")), Facts);
    }
    if (MaterialPath.IsEmpty())
    {
        return WriteReceipt(false, PackagePath, MaterialPath, OneError(TEXT("material_path is required; the bridge refuses to author an unmaterialed template.")), Facts);
    }

    UNiagaraEmitter* MinimalEmitter = NewObject<UNiagaraEmitter>(GetTransientPackage(), NAME_None, RF_Transactional);
    UNiagaraEmitterFactoryNew::InitializeEmitter(MinimalEmitter,true);
    UNiagaraScript* SpawnBurst = LoadObject<UNiagaraScript>(nullptr, SpawnBurstPath);
    UNiagaraScript* AddVelocity = LoadObject<UNiagaraScript>(nullptr, AddVelocityPath);
    UMaterialInterface* AmberMaterial = LoadObject<UMaterialInterface>(nullptr, *MaterialPath);
    if (!MinimalEmitter || !SpawnBurst || !AddVelocity || !AmberMaterial)
    {
        TArray<FString> Errors;
        if (!MinimalEmitter) { Errors.Add(FString::Printf(TEXT("Minimal emitter template unavailable: %s"), MinimalEmitterPath)); }
        if (!SpawnBurst) { Errors.Add(FString::Printf(TEXT("SpawnBurst module unavailable: %s"), SpawnBurstPath)); }
        if (!AddVelocity) { Errors.Add(FString::Printf(TEXT("AddVelocity module unavailable: %s"), AddVelocityPath)); }
        if (!AmberMaterial) { Errors.Add(FString::Printf(TEXT("Amber material unavailable or not a UMaterialInterface: %s"), *MaterialPath)); }
        return WriteReceipt(false, PackagePath, MaterialPath, Errors, Facts);
    }

    const FString AssetPath = FPackageName::GetLongPackagePath(PackagePath);
    const FString AssetName = FPackageName::GetLongPackageAssetName(PackagePath);
    FNiagaraExternalEditContext CreateContext;
    UNiagaraSystem* System = UNiagaraExternalEditUtilities::CreateNiagaraSystem(AssetName, AssetPath, nullptr, CreateContext);
    if (!System || CreateContext.HasErrors())
    {
        return WriteReceipt(false, PackagePath, MaterialPath, ContextErrors(CreateContext), Facts);
    }

    FNiagaraExternalEditContext Context(System);
    FNiagaraExt_EmitterTopology Topology;
    const FName EmitterName(TEXT("RiftShard"));
    UNiagaraExternalEditUtilities::AddEmitter(MinimalEmitter, EmitterName, Topology, Context);
    if (Context.HasErrors() || Topology.EmitterName == NAME_None)
    {
        return WriteReceipt(false, PackagePath, MaterialPath, ContextErrors(Context), Facts);
    }

    const FNiagaraExt_ModuleTopology* SpawnRate = FindModule(Topology.EmitterUpdateScript, TEXT("SpawnRate"));
    if (!SpawnRate)
    {
        return WriteReceipt(false, PackagePath, MaterialPath, OneError(TEXT("Minimal emitter topology has no SpawnRate module; refusing a topology-unknown authoring result.")), Facts);
    }
    FNiagaraExt_StackItemReference SpawnRateRef(System, EmitterName, Topology.EmitterUpdateScript.ScriptName, SpawnRate->ModuleName);
    UNiagaraExternalEditUtilities::RemoveModule(SpawnRateRef, Context);
    if (Context.HasErrors() || !RefreshTopology(System, EmitterName, Topology, Context))
    {
        return WriteReceipt(false, PackagePath, MaterialPath, ContextErrors(Context), Facts);
    }

    FNiagaraExt_StackItemReference EmitterUpdateRef(System, EmitterName, Topology.EmitterUpdateScript.ScriptName);
    FNiagaraExt_ModuleTopology BurstTopology;
    UNiagaraExternalEditUtilities::AddModule(EmitterUpdateRef, SpawnBurst, BurstTopology, Context);
    if (Context.HasErrors() || BurstTopology.ModuleName == NAME_None ||
        !SetFloatInput(System, EmitterName, Topology.EmitterUpdateScript.ScriptName, BurstTopology, {TEXT("Spawn Count"), TEXT("Spawn Count Min")}, 1.0f, Context))
    {
        return WriteReceipt(false, PackagePath, MaterialPath, ContextErrors(Context), Facts);
    }

    if (!RefreshTopology(System, EmitterName, Topology, Context))
    {
        return WriteReceipt(false, PackagePath, MaterialPath, ContextErrors(Context), Facts);
    }
    const FNiagaraExt_ModuleTopology* Velocity = FindModule(Topology.ParticleSpawnScript, TEXT("AddVelocity"));
    FNiagaraExt_ModuleTopology AddedVelocity;
    if (!Velocity)
    {
        FNiagaraExt_StackItemReference ParticleSpawnRef(System, EmitterName, Topology.ParticleSpawnScript.ScriptName);
        UNiagaraExternalEditUtilities::AddModule(ParticleSpawnRef, AddVelocity, AddedVelocity, Context);
        if (Context.HasErrors() || AddedVelocity.ModuleName == NAME_None)
        {
            return WriteReceipt(false, PackagePath, MaterialPath, ContextErrors(Context), Facts);
        }
        Velocity = &AddedVelocity;
    }
    if (!SetVelocityInput(System, EmitterName, Topology.ParticleSpawnScript.ScriptName, *Velocity, Context))
    {
        return WriteReceipt(false, PackagePath, MaterialPath, ContextErrors(Context), Facts);
    }

    if (!RefreshTopology(System, EmitterName, Topology, Context))
    {
        return WriteReceipt(false, PackagePath, MaterialPath, ContextErrors(Context), Facts);
    }
    const FNiagaraExt_ModuleTopology* LifetimeModule=nullptr;
    for (const auto& Module : Topology.ParticleSpawnScript.Modules)
    {
        if (FindInput(Module,{TEXT("Particles.Lifetime"),TEXT("Lifetime")})) { LifetimeModule=&Module;break; }
    }
    if (!LifetimeModule || !SetFloatInput(System,EmitterName,Topology.ParticleSpawnScript.ScriptName,*LifetimeModule,{TEXT("Particles.Lifetime"),TEXT("Lifetime")},.5f,Context))
    {
        if (!LifetimeModule) { Context.Error(FText::FromString(TEXT("No explicit spawn lifetime input found."))); }
        return WriteReceipt(false,PackagePath,MaterialPath,ContextErrors(Context),Facts);
    }

    if (!RefreshTopology(System, EmitterName, Topology, Context) || Topology.Renderers.Num() != 1)
    {
        if (!Context.HasErrors()) { Context.Error(FText::FromString(TEXT("Minimal emitter must resolve to exactly one renderer; refusing unknown renderer topology."))); }
        return WriteReceipt(false, PackagePath, MaterialPath, ContextErrors(Context), Facts);
    }
    FNiagaraEmitterHandle* EmitterHandle = System->GetEmitterHandles().FindByPredicate(
        [EmitterName](const FNiagaraEmitterHandle& Handle) { return Handle.GetName() == EmitterName; });
    FVersionedNiagaraEmitterData* EmitterData = EmitterHandle ? EmitterHandle->GetEmitterData() : nullptr;
    const TArray<UNiagaraRendererProperties*>* Renderers = EmitterData ? &EmitterData->GetRenderers() : nullptr;
    UNiagaraSpriteRendererProperties* SpriteRenderer = Renderers && Renderers->Num() == 1
        ? Cast<UNiagaraSpriteRendererProperties>((*Renderers)[0]) : nullptr;
    if (!SpriteRenderer)
    {
        Context.Error(FText::FromString(TEXT("Minimal emitter renderer is not exactly one sprite renderer; refusing to silently substitute another renderer.")));
        return WriteReceipt(false, PackagePath, MaterialPath, ContextErrors(Context), Facts);
    }

    System->Modify();
    System->bFixedBounds = true;
    System->SetFixedBounds(FBox(FVector(-620.0, -620.0, -620.0), FVector(620.0, 620.0, 620.0)));
    System->bOverrideCastShadow = true;
    System->bCastShadow = false;
    SpriteRenderer->Modify();
    EmitterData->bLocalSpace=false;
    EmitterData->bDeterminism=true;
    SpriteRenderer->Alignment=ENiagaraSpriteAlignment::VelocityAligned;
    SpriteRenderer->Material = AmberMaterial;
    SpriteRenderer->bCastShadows = false;
    System->MarkPackageDirty();

    System->RequestCompile(true);
    System->WaitForCompilationComplete(true, false);
    FNiagaraExt_SystemCompileState CompileState;
    UNiagaraExternalEditUtilities::GetSystemCompileState(System, CompileState, Context);
    if (Context.HasErrors() || CompileState.bHasErrors || CompileState.bIsCompiling || CompileState.bIsStale)
    {
        if (CompileState.bHasErrors) { Context.Error(FText::FromString(TEXT("Niagara compilation returned script errors."))); }
        if (CompileState.bIsCompiling || CompileState.bIsStale) { Context.Error(FText::FromString(TEXT("Niagara compilation did not settle before receipt creation."))); }
        return WriteReceipt(false, PackagePath, MaterialPath, ContextErrors(Context), Facts);
    }

    UEditorAssetSubsystem* Assets = GEditor ? GEditor->GetEditorSubsystem<UEditorAssetSubsystem>() : nullptr;
    if (!Assets || !Assets->SaveLoadedAsset(System, false))
    {
        return WriteReceipt(false, PackagePath, MaterialPath, OneError(TEXT("Compilation completed, but the editor asset subsystem could not save the sandbox Niagara system.")), Facts);
    }
    Facts.Add(TEXT("compile"), TEXT("completed without reported script errors"));
    Facts.Add(TEXT("save"), TEXT("completed through UEditorAssetSubsystem"));
    Facts.Add(TEXT("performance_validation"), TEXT("NOT_RUN"));
    return WriteReceipt(true, PackagePath, MaterialPath, {}, Facts);
}


#include "NiagaraComponent.h"
#include "NiagaraSystemInstanceController.h"
#include "NiagaraSystemInstance.h"
#include "NiagaraEmitterInstance.h"
#include "NiagaraDataSetAccessor.h"

FString URiftNiagaraAuthoringLibrary::ReadParticles(UNiagaraComponent* Component)
{
    TSharedRef<FJsonObject> Root=MakeShared<FJsonObject>();
    Root->SetStringField(TEXT("evidence"),TEXT("CPU Niagara current particle dataset after concurrent tick finalization"));
    TArray<TSharedPtr<FJsonValue>> Rows;
    auto Controller=Component ? Component->GetSystemInstanceController() : nullptr;
    if (!Controller.IsValid()) { Root->SetStringField(TEXT("error"),TEXT("No system instance controller")); }
    else
    {
        Controller->WaitForConcurrentTickAndFinalize();
        auto* Instance=Controller->GetSystemInstance_Unsafe();
        if (Instance)
        {
            for (const auto& Emitter : Instance->GetEmitters())
            {
                int32 Count=Emitter->GetNumParticles();
                if (Count<=0) continue;
                const auto& Data=Emitter->GetData();
                auto Position=FNiagaraDataSetAccessor<FNiagaraPosition>(Data,TEXT("Position")).GetReader(Data);
                auto Velocity=FNiagaraDataSetAccessor<FVector3f>(Data,TEXT("Velocity")).GetReader(Data);
                if (!Position.IsValid() || !Velocity.IsValid()) { Root->SetStringField(TEXT("error"),TEXT("Particle position/velocity accessor unavailable")); continue; }
                for (int32 i=0;i<Count;++i)
                {
                    auto P=Position.Get(i);auto V=Velocity.Get(i);
                    TSharedRef<FJsonObject> Row=MakeShared<FJsonObject>();
                    Row->SetNumberField(TEXT("x_cm"),P.X);Row->SetNumberField(TEXT("y_cm"),P.Y);Row->SetNumberField(TEXT("z_cm"),P.Z);
                    Row->SetNumberField(TEXT("vx_cm_s"),V.X);Row->SetNumberField(TEXT("vy_cm_s"),V.Y);Row->SetNumberField(TEXT("vz_cm_s"),V.Z);
                    Rows.Add(MakeShared<FJsonValueObject>(Row));
                }
            }
        }
    }
    Root->SetArrayField(TEXT("particles"),Rows);
    FString Json;auto Writer=TJsonWriterFactory<>::Create(&Json);FJsonSerializer::Serialize(Root,Writer);return Json;
}

#include "AssetCompilingManager.h"
#include "ShaderCompiler.h"
#include "RenderingThread.h"
void URiftNiagaraAuthoringLibrary::FinishPreviewCompilation()
{
    FAssetCompilingManager::Get().FinishAllCompilation();
    if (GShaderCompilingManager) { GShaderCompilingManager->FinishAllCompilation(); }
    FlushRenderingCommands();
}

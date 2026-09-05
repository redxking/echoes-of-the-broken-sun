#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "EchoesTerrainView.h"
#include "EchoesCompiledMapBinding.h"
#include "Components/PointLightComponent.h"
#include "EchoesTestSaveEnvironment.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "ProceduralMeshComponent.h"
#include "StaticMeshResources.h"
#include "Engine/World.h"
#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Tests/AutomationCommon.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEchoesWorldKitTest,
    "Echoes.Runtime.Map.WorldKitVisibility",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::EngineFilter)

bool FEchoesWorldKitTest::RunTest(const FString& Parameters)
{
    FEchoesScopedTestSaveEnvironment SaveEnvironment(*this);
    if (!SaveEnvironment.IsReady()) return false;
    FTestWorldWrapper Wrapper;
    if (!Wrapper.CreateTestWorld(EWorldType::Game)) return false;
    const EEchoesOperationMode Modes[] = {
        EEchoesOperationMode::CampaignPrologue, EEchoesOperationMode::CampaignSevenAccounts,
        EEchoesOperationMode::CampaignUnburiedRoad, EEchoesOperationMode::CampaignNamesWithoutBirths,
        EEchoesOperationMode::CampaignShapeBesideUs, EEchoesOperationMode::CampaignTheBrokenSun};
    TSet<FString> MeshNames;
    for (const auto Mode : Modes)
    {
        AEchoesTerrainView* View = Wrapper.GetTestWorld()->SpawnActor<AEchoesTerrainView>();
        if (!TestNotNull(TEXT("terrain actor"), View)) return false;
        if (!TestTrue(TEXT("registered world kit initializes"),
            View->InitializeScopedTerrain(64, 64, 200, EEchoesSkirmishMapPreset::GlassScar, Mode))) return false;
        UInstancedStaticMeshComponent* Ground = nullptr;
        UInstancedStaticMeshComponent* Surface = nullptr;
        UInstancedStaticMeshComponent* Formation = nullptr;
        UInstancedStaticMeshComponent* Exterior = nullptr;
        TArray<UInstancedStaticMeshComponent*> Layers;
        View->GetComponents(Layers);
        for (auto* Layer : Layers)
        {
            if (Layer->GetName() == TEXT("BiomeGround")) Ground = Layer;
            if (Layer->GetName() == TEXT("BiomeSurface")) Surface = Layer;
            if (Layer->GetName() == TEXT("BlockedTiles")) Formation = Layer;
            if (Layer->GetName() == TEXT("M01ExteriorSkirt")) Exterior = Layer;
            TestTrue(TEXT("terrain has no collision"), Layer->GetCollisionEnabled() == ECollisionEnabled::NoCollision);
            TestFalse(TEXT("terrain cannot affect navigation"), Layer->CanEverAffectNavigation());
            TestFalse(TEXT("terrain cannot overlap"), Layer->GetGenerateOverlapEvents());
            TestFalse(TEXT("terrain does not cast shadows"), Layer->CastShadow);
            if (Layer->GetName() == TEXT("BiomeHorizon"))
            {
                for (int32 Slot = 0; Slot < 4; ++Slot)
                {
                    UMaterialInterface* Material = Layer->GetMaterial(Slot);
                    if (Mode == EEchoesOperationMode::CampaignPrologue)
                        TestTrue(TEXT("M01 public vertical basalt uses its registered cliff surface"),
                            Material && Material->GetPathName() ==
                                TEXT("/Game/Art/Generated/Materials/M_EchoesCliffSurface.M_EchoesCliffSurface"));
                    else
                        TestNotNull(TEXT("other operations retain their existing dynamic horizon material"),
                            Cast<UMaterialInstanceDynamic>(Material));
                }
                for (int32 Index = 0; Index < Layer->GetInstanceCount(); ++Index)
                {
                    FTransform Transform;
                    Layer->GetInstanceTransform(Index, Transform);
                    const FBox Bounds = Layer->GetStaticMesh()->GetBoundingBox().TransformBy(Transform);
                    TestTrue(TEXT("public scenery stays outside every playable tile"),
                        Bounds.Max.X < -6500 || Bounds.Min.X > 6300 ||
                        Bounds.Max.Y < -6500 || Bounds.Min.Y > 6300);
                }
            }
        }
        if (!TestNotNull(TEXT("scoped ground layer"), Ground) ||
            !TestNotNull(TEXT("authored substrate layer"), Surface) ||
            !TestNotNull(TEXT("scoped formation layer"), Formation)) return false;
        if (!TestNotNull(TEXT("separate public exterior substrate"), Exterior)) return false;
        TestEqual(TEXT("only M01 authors an exterior substrate"), Exterior->GetInstanceCount(),
            Mode == EEchoesOperationMode::CampaignPrologue ? 4 : 0);
        TArray<FTransform> ExteriorBefore;
        TSet<int32> AbuttingEdges;
        for (int32 Index = 0; Index < Exterior->GetInstanceCount(); ++Index)
        {
            FTransform Instance;
            Exterior->GetInstanceTransform(Index, Instance);
            ExteriorBefore.Add(Instance);
            const FBox Bounds = Exterior->GetStaticMesh()->GetBoundingBox().TransformBy(Instance);
            TestEqual(TEXT("exterior reuses the registered walking substrate"),
                Exterior->GetStaticMesh()->GetName(), FString(TEXT("SM_World_WalkSurface")));
            TestTrue(TEXT("public substrate touches the boundary without entering a playable cell"),
                Bounds.Max.X <= -6500+.01 || Bounds.Min.X >= 6300-.01 ||
                Bounds.Max.Y <= -6500+.01 || Bounds.Min.Y >= 6300-.01);
            TestTrue(TEXT("public substrate retains its two-centimetre depth"),
                FMath::IsNearlyEqual(Bounds.GetSize().Z, 2.0, .01));
            if (FMath::IsNearlyEqual(Bounds.Max.Y, -6500.0, .01) && Bounds.Min.X <= -6500 && Bounds.Max.X >= 6300) AbuttingEdges.Add(0);
            if (FMath::IsNearlyEqual(Bounds.Min.X, 6300.0, .01) && Bounds.Min.Y <= -6500 && Bounds.Max.Y >= 6300) AbuttingEdges.Add(1);
            if (FMath::IsNearlyEqual(Bounds.Min.Y, 6300.0, .01) && Bounds.Min.X <= -6500 && Bounds.Max.X >= 6300) AbuttingEdges.Add(2);
            if (FMath::IsNearlyEqual(Bounds.Max.X, -6500.0, .01) && Bounds.Min.Y <= -6500 && Bounds.Max.Y >= 6300) AbuttingEdges.Add(3);
        }
        if (Mode == EEchoesOperationMode::CampaignPrologue)
            TestEqual(TEXT("all four complete edges are supported without a background gap"), AbuttingEdges.Num(), 4);
        const auto CheckPublicExteriorUnchanged = [&]()
        {
            TestEqual(TEXT("fog changes cannot create public exterior instances"), Exterior->GetInstanceCount(), ExteriorBefore.Num());
            for (int32 Index = 0; Index < ExteriorBefore.Num(); ++Index)
            {
                FTransform Instance;
                Exterior->GetInstanceTransform(Index, Instance);
                TestTrue(TEXT("public exterior does not expose private terrain through a changing silhouette"),
                    Instance.Equals(ExteriorBefore[Index]));
            }
        };
        MeshNames.Add(Formation->GetStaticMesh()->GetName());
        auto* SurfaceMaterial = Cast<UMaterialInstanceDynamic>(Surface->GetMaterial(0));
        float WorldUVScale = 0.0f;
        TestTrue(TEXT("substrate has authored world texture scale"), SurfaceMaterial &&
            SurfaceMaterial->GetScalarParameterValue(FMaterialParameterInfo(TEXT("WorldUVScale")), WorldUVScale));
        TestEqual(TEXT("M01 uses 25m texture period; other operations retain their prior scale"),
            WorldUVScale, Mode == EEchoesOperationMode::CampaignPrologue ? .0004f : .0012f);
        const auto HasVisibleFormation = [View, Formation]()
        {
            FTransform Instance;
            Formation->GetInstanceTransform(0, Instance);
            if (!Instance.GetScale3D().IsNearlyZero()) return true;
            UProceduralMeshComponent* Cliffs = nullptr;
            TArray<UProceduralMeshComponent*> ProceduralLayers;
            View->GetComponents(ProceduralLayers);
            for (auto* Layer : ProceduralLayers)
                if (Layer->GetName() == TEXT("M01ContinuousCliffs")) Cliffs = Layer;
            if (Cliffs)
                for (int32 Section=0; Section<Cliffs->GetNumSections(); ++Section)
                    if (const FProcMeshSection* Mesh = Cliffs->GetProcMeshSection(Section))
                        if (!Mesh->ProcVertexBuffer.IsEmpty()) return true;
            return false;
        };
        if (Mode == EEchoesOperationMode::CampaignSevenAccounts)
        {
            auto* Material = Cast<UMaterialInstanceDynamic>(Surface->GetMaterial(0));
            if (!TestNotNull(TEXT("grassland substrate material"),Material)) return false;
            UTexture* Texture = nullptr;
            TestTrue(TEXT("grassland has a bound base texture"),
                Material->GetTextureParameterValue(FMaterialParameterInfo(TEXT("GroundBaseColorMap")),Texture));
            TestTrue(TEXT("grassland uses ash substrate instead of Glass Scar fissures"),
                Texture && Texture->GetName() == TEXT("T_EchoesCausewayAsh_BaseColor"));
            float Glow = -1;
            TestTrue(TEXT("grassland exposes ground glow control"),
                Material->GetScalarParameterValue(FMaterialParameterInfo(TEXT("GroundGlowStrength")),Glow));
            TestEqual(TEXT("grassland has no false molten fissures"),Glow,0.0f);
            const FStaticMeshRenderData* LeafRenderData = Ground->GetStaticMesh()->GetRenderData();
            if (!TestNotNull(TEXT("leaf mesh has render data"), LeafRenderData)) return false;
            for (const FStaticMeshLODResources& Lod : LeafRenderData->LODResources)
                for (const FStaticMeshSection& Section : Lod.Sections)
                    if (Section.NumTriangles > 0)
                        TestEqual(TEXT("leaf LOD actually draws with the pale material zone"), Section.MaterialIndex, 2);
            auto* Leaves = Cast<UMaterialInstanceDynamic>(Ground->GetMaterial(2));
            if (!TestNotNull(TEXT("leaf ribbon material"), Leaves)) return false;
            FLinearColor LeafColor;
            TestTrue(TEXT("leaves carry the silver foliage palette"),
                Leaves->GetVectorParameterValue(FMaterialParameterInfo(TEXT("Color")), LeafColor) &&
                LeafColor.Equals(FLinearColor(.52f,.50f,.53f), .001f));
            TestTrue(TEXT("thin leaves light from both faces"), Leaves->IsTwoSided());
            TestTrue(TEXT("thin leaves use foliage transmission"),
                Leaves->GetShadingModels().HasShadingModel(MSM_TwoSidedFoliage));
        }
        std::vector<echoes::sim::net::ScopedTileState> Tiles(4096);
        for (auto& Tile : Tiles)
        {
            Tile.terrain = echoes::sim::Terrain::Blocked;
            Tile.visibility = echoes::sim::Visibility::Unexplored;
        }
        TestTrue(TEXT("unexplored terrain sync"), View->SyncScopedTerrain(Tiles));
        TestEqual(TEXT("unexplored fixture draws no private blocked cells"), View->GetInstancedBlockedTileCount(), 0);
        CheckPublicExteriorUnchanged();
        FTransform Transform;
        Formation->GetInstanceTransform(0, Transform);
        TestFalse(TEXT("hidden formations disclose no height"), HasVisibleFormation());
        Ground->GetInstanceTransform(0, Transform);
        TestTrue(TEXT("hidden ground discloses no detail"), Transform.GetScale3D().IsNearlyZero());
        Surface->GetInstanceTransform(0, Transform);
        TestTrue(TEXT("hidden substrate discloses no terrain"), Transform.GetScale3D().IsNearlyZero());
        Tiles[0].visibility = echoes::sim::Visibility::Visible;
        View->SyncScopedTerrain(Tiles);
        CheckPublicExteriorUnchanged();
        Formation->GetInstanceTransform(0, Transform);
        TestTrue(TEXT("visible blocked cell shows its formation"), HasVisibleFormation());
        Tiles[0].terrain = echoes::sim::Terrain::Open;
        View->SyncScopedTerrain(Tiles);
        Formation->GetInstanceTransform(0, Transform);
        TestFalse(TEXT("opening a cell removes the tall formation"), HasVisibleFormation());
        Ground->GetInstanceTransform(0, Transform);
        TestFalse(TEXT("open cell retains only low relief"), Transform.GetScale3D().IsNearlyZero());
        Surface->GetInstanceTransform(0, Transform);
        TestFalse(TEXT("known open cell has an authored substrate"), Transform.GetScale3D().IsNearlyZero());
        Tiles[0].visibility = echoes::sim::Visibility::Unexplored;
        View->SyncScopedTerrain(Tiles);
        CheckPublicExteriorUnchanged();
        Ground->GetInstanceTransform(0, Transform);
        TestTrue(TEXT("visibility reset conceals ground again"), Transform.GetScale3D().IsNearlyZero());
        Surface->GetInstanceTransform(0, Transform);
        TestTrue(TEXT("visibility reset conceals substrate again"), Transform.GetScale3D().IsNearlyZero());
        View->Destroy();
    }
    TestEqual(TEXT("six biomes have distinct formation geometry"), MeshNames.Num(), 6);
    // The chasm uses separate meshes and lights; tile-only fog checks cannot
    // establish that it respects the same information boundary.
    echoes::sim::SimulationConfig Config;
    Config.rules = echoes::sim::DefaultSimulationRules();
    echoes::sim::Simulation Simulation(Config);
    echoes::world::ApplyCompiledGlassScar(Simulation);
    AStaticMeshActor* Route = Wrapper.GetTestWorld()->SpawnActor<AStaticMeshActor>();
    Route->Tags.Add(TEXT("EchoesRouteFoldedVerge"));
    Route->GetStaticMeshComponent()->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,
        TEXT("/Game/Art/Generated/World/Environment/SM_World_GlassScarFoldedVerge")));
    Route->SetActorLocation(FVector(3400, 0, 20));
    AEchoesTerrainView* Chasm = Wrapper.GetTestWorld()->SpawnActor<AEchoesTerrainView>();
    if (!Chasm || !TestTrue(TEXT("chasm fixture initializes"),
        Chasm->InitializeTerrain(Simulation, 200, EEchoesSkirmishMapPreset::GlassScar,
            std::nullopt, EEchoesOperationMode::CampaignPrologue))) return false;
    TestTrue(TEXT("fixture actually contains chasm geometry"), Chasm->HasChasmComposition());
    TArray<UInstancedStaticMeshComponent*> ChasmLayers;
    Chasm->GetComponents(ChasmLayers);
    UInstancedStaticMeshComponent* M01Bed = nullptr;
    for (auto* Layer : ChasmLayers)
        if (Layer->GetName() == TEXT("ChasmBed")) M01Bed = Layer;
    if (!TestNotNull(TEXT("M01 registered bed layer"), M01Bed)) return false;
    for (int32 Slot = 0; Slot < 4; ++Slot)
        TestTrue(TEXT("M01 cut faces bind the registered non-emissive 3D basalt master"),
            M01Bed->GetMaterial(Slot) && M01Bed->GetMaterial(Slot)->GetPathName() ==
                TEXT("/Game/Art/Generated/Materials/M_EchoesCliffSurface.M_EchoesCliffSurface"));
    auto* LegacyChasm = Wrapper.GetTestWorld()->SpawnActor<AEchoesTerrainView>();
    if (!TestNotNull(TEXT("non-M01 chasm control"), LegacyChasm) ||
        !TestTrue(TEXT("explicit Glass Scar skirmish retains its composition"),
            LegacyChasm->InitializeTerrain(Simulation, 200, EEchoesSkirmishMapPreset::GlassScar,
                std::nullopt, EEchoesOperationMode::Skirmish))) return false;
    TArray<UInstancedStaticMeshComponent*> LegacyLayers;
    LegacyChasm->GetComponents(LegacyLayers);
    UInstancedStaticMeshComponent* LegacyBed = nullptr;
    for (auto* Layer : LegacyLayers) if (Layer->GetName() == TEXT("ChasmBed")) LegacyBed = Layer;
    if (!TestNotNull(TEXT("non-M01 bed control"), LegacyBed)) return false;
    TestNotNull(TEXT("non-M01 bed retains its existing dynamic world material"),
        Cast<UMaterialInstanceDynamic>(LegacyBed->GetMaterial(0)));
    TestTrue(TEXT("M01 basalt assignment does not bleed into skirmish"),
        LegacyBed->GetMaterial(0) != M01Bed->GetMaterial(0));
    LegacyChasm->Destroy();
    TArray<UPointLightComponent*> Lights;
    Chasm->GetComponents(Lights);
    TestTrue(TEXT("fixture actually contains fissure lights"), Lights.Num() > 0);
    std::vector<echoes::sim::net::ScopedTileState> ChasmTiles(4096);
    for (int32 Y = 0; Y < 64; ++Y)
        for (int32 X = 0; X < 64; ++X)
            ChasmTiles[Y * 64 + X].terrain = Simulation.TerrainAt(X, Y);
    for (const auto Visibility : {echoes::sim::Visibility::Unexplored,
         echoes::sim::Visibility::Visible, echoes::sim::Visibility::Explored,
         echoes::sim::Visibility::Unexplored})
    {
        for (auto& Tile : ChasmTiles) Tile.visibility = Visibility;
        TestTrue(TEXT("chasm scoped transition sync"), Chasm->SyncScopedTerrain(ChasmTiles));
        const bool bKnown = Visibility != echoes::sim::Visibility::Unexplored;
        TestEqual(TEXT("M01 crossing details follow complete remembered footprint"), !Route->IsHidden(), bKnown);
        for (auto* Layer : ChasmLayers)
        {
            if (!Layer->GetName().StartsWith(TEXT("Chasm"))) continue;
            for (int32 Index = 0; Index < Layer->GetInstanceCount(); ++Index)
            {
                FTransform Transform;
                Layer->GetInstanceTransform(Index, Transform);
                TestEqual(TEXT("chasm instances follow known terrain"),
                    !Transform.GetScale3D().IsNearlyZero(), bKnown);
            }
        }
        for (auto* Light : Lights)
            TestEqual(TEXT("fissure lights follow known terrain"), Light->IsVisible(), bKnown);
    }
    ChasmTiles[0].visibility = echoes::sim::Visibility::Visible;
    Chasm->SyncScopedTerrain(ChasmTiles);
    for (auto* Light : Lights) TestFalse(TEXT("one known tile cannot disclose a fissure"), Light->IsVisible());
    TestTrue(TEXT("one known tile cannot disclose an M01 crossing"), Route->IsHidden());
    Route->Destroy();
    Chasm->Destroy();
    Wrapper.ForwardErrorMessages(this);
    return true;
}
#endif

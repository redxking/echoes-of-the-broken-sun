#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "EchoesTestSaveEnvironment.h"

#include "EchoesFogView.h"
#include "EchoesPlayerController.h"
#include "EchoesSimCore/Simulation.h"
#include "EchoesSimulationSubsystem.h"
#include "EchoesSkirmishOverlayLayout.h"
#include "EchoesTerrainView.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Tests/AutomationCommon.h"

#include <algorithm>

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEchoesSkirmishSetupTest,
    "Echoes.Runtime.Gameplay.SkirmishSetup",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::ClientContext |
        EAutomationTestFlags::EngineFilter)

bool FEchoesSkirmishSetupTest::RunTest(const FString& Parameters)
{
    (void)Parameters;

    FEchoesScopedTestSaveEnvironment TestSaveEnvironment(*this);
    if (!TestSaveEnvironment.IsReady())
    {
        return false;
    }

    const echoes::sim::Faction Factions[] = {
        echoes::sim::Faction::MeridianCompact,
        echoes::sim::Faction::KharuunAssemblies,
        echoes::sim::Faction::HollowChoir};
    const EEchoesSkirmishMapPreset Maps[] = {
        EEchoesSkirmishMapPreset::GlassScar,
        EEchoesSkirmishMapPreset::CrownfallBasin,
        EEchoesSkirmishMapPreset::SorynConfluence};
    const echoes::sim::AiPersonality Profiles[] = {
        echoes::sim::AiPersonality::Economic,
        echoes::sim::AiPersonality::Defensive,
        echoes::sim::AiPersonality::Raider};
    const EEchoesSkirmishResourceLevel ResourceLevels[] = {
        EEchoesSkirmishResourceLevel::Scarce,
        EEchoesSkirmishResourceLevel::Standard,
        EEchoesSkirmishResourceLevel::Abundant};
    const echoes::sim::ResearchType IdentityResearch[] = {
        echoes::sim::ResearchType::MeridianPrismaticTargeting,
        echoes::sim::ResearchType::KharuunEchoCartography,
        echoes::sim::ResearchType::ChoirHeldAlternatives};

    TestEqual(
        TEXT("Glass Scar exposes its exact player-facing map name"),
        FString(FEchoesSkirmishSetupModel::MapDisplayName(
            EEchoesSkirmishMapPreset::GlassScar)),
        FString(TEXT("GLASS SCAR")));
    TestEqual(
        TEXT("Crownfall Basin exposes its exact player-facing map name"),
        FString(FEchoesSkirmishSetupModel::MapDisplayName(
            EEchoesSkirmishMapPreset::CrownfallBasin)),
        FString(TEXT("CROWNFALL BASIN")));
    TestEqual(
        TEXT("Soryn Confluence exposes its exact player-facing map name"),
        FString(FEchoesSkirmishSetupModel::MapDisplayName(
            EEchoesSkirmishMapPreset::SorynConfluence)),
        FString(TEXT("SORYN CONFLUENCE")));

    TSet<int32> DistinctBlockedCounts;
    for (int32 MapIndex = 0; MapIndex < UE_ARRAY_COUNT(Maps); ++MapIndex)
    {
        DistinctBlockedCounts.Add(
            FEchoesSkirmishSetupModel::ExpectedBlockedTileCount(
                Maps[MapIndex]));
        for (int32 FactionIndex = 0;
             FactionIndex < UE_ARRAY_COUNT(Factions);
             ++FactionIndex)
        {
            FEchoesSkirmishSetup Setup;
            Setup.LocalFaction = Factions[FactionIndex];
            Setup.OpponentFaction =
                Factions[(FactionIndex + 1) % UE_ARRAY_COUNT(Factions)];
            Setup.MapPreset = Maps[MapIndex];
            FString ValidationError;
            TestTrue(
                FString::Printf(
                    TEXT("%s supports the complete %s deployment footprint and route contract"),
                    FEchoesSkirmishSetupModel::MapDisplayName(Maps[MapIndex]),
                    FEchoesSkirmishSetupModel::FactionDisplayName(
                        Factions[FactionIndex])),
                FEchoesSkirmishSetupModel::Validate(
                    Setup, ValidationError));
        }
    }
    TestEqual(
        TEXT("Glass Scar keeps its authored blocked-tile contract"),
        FEchoesSkirmishSetupModel::ExpectedBlockedTileCount(
            EEchoesSkirmishMapPreset::GlassScar),
        165);
    TestEqual(
        TEXT("Crownfall Basin keeps its authored blocked-tile contract"),
        FEchoesSkirmishSetupModel::ExpectedBlockedTileCount(
            EEchoesSkirmishMapPreset::CrownfallBasin),
        282);
    TestEqual(
        TEXT("Soryn Confluence keeps its authored blocked-tile contract"),
        FEchoesSkirmishSetupModel::ExpectedBlockedTileCount(
            EEchoesSkirmishMapPreset::SorynConfluence),
        200);
    TestEqual(TEXT("All three battlefields have distinct terrain patterns"),
              DistinctBlockedCounts.Num(), 3);
    TestTrue(
        TEXT("Battlefields retain distinct deployment geometry"),
        FEchoesSkirmishSetupModel::LocalSpawnTiles(Maps[0]) !=
                FEchoesSkirmishSetupModel::LocalSpawnTiles(Maps[1]) &&
            FEchoesSkirmishSetupModel::LocalSpawnTiles(Maps[1]) !=
                FEchoesSkirmishSetupModel::LocalSpawnTiles(Maps[2]));
    TestTrue(
        TEXT("Starting-resource profiles are materially distinct"),
        FEchoesSkirmishSetupModel::StartingResources(ResourceLevels[0]) ==
                echoes::sim::ResourcePool{320, 18} &&
            FEchoesSkirmishSetupModel::StartingResources(ResourceLevels[1]) ==
                echoes::sim::ResourcePool{500, 30} &&
            FEchoesSkirmishSetupModel::StartingResources(ResourceLevels[2]) ==
                echoes::sim::ResourcePool{800, 60});

    const FVector2D LayoutViewports[] = {
        FVector2D(1280.0f, 720.0f),
        FVector2D(1600.0f, 900.0f),
        FVector2D(1920.0f, 1080.0f)};
    for (const FVector2D& Viewport : LayoutViewports)
    {
        const FEchoesSkirmishSetupOverlayLayout SetupLayout =
            FEchoesSkirmishSetupOverlayLayout::Build(Viewport, 1.0f);
        const FEchoesSkirmishSummaryOverlayLayout SummaryLayout =
            FEchoesSkirmishSummaryOverlayLayout::Build(Viewport, 1.0f);
        const FEchoesPauseOverlayLayout PauseLayout =
            FEchoesPauseOverlayLayout::Build(Viewport, 1.0f);
        const FEchoesResultOverlayLayout ResultLayout =
            FEchoesResultOverlayLayout::Build(Viewport, 1.0f);
        const auto InsideViewport = [&Viewport](const FBox2D& Box)
        {
            return Box.Min.X >= 0.0f && Box.Min.Y >= 0.0f &&
                Box.Max.X <= Viewport.X && Box.Max.Y <= Viewport.Y;
        };
        bool bSetupTargetsSafe = InsideViewport(SetupLayout.ReviewButton);
        for (int32 Row = 0; Row < 5; ++Row)
        {
            bSetupTargetsSafe &=
                InsideViewport(SetupLayout.SettingRows[Row]) &&
                SetupLayout.SettingDecrease[Row].Intersect(
                    SetupLayout.SettingRows[Row]) &&
                SetupLayout.SettingIncrease[Row].Intersect(
                    SetupLayout.SettingRows[Row]);
            if (Row > 0)
            {
                bSetupTargetsSafe &=
                    !SetupLayout.SettingRows[Row - 1].Intersect(
                        SetupLayout.SettingRows[Row]);
            }
        }
        TestTrue(
            FString::Printf(
                TEXT("Setup pointer targets remain layout-safe at %.0fx%.0f"),
                Viewport.X,
                Viewport.Y),
            bSetupTargetsSafe);
        TestTrue(
            FString::Printf(
                TEXT("Summary pointer targets remain distinct at %.0fx%.0f"),
                Viewport.X,
                Viewport.Y),
            InsideViewport(SummaryLayout.BackButton) &&
                InsideViewport(SummaryLayout.DeployButton) &&
                !SummaryLayout.BackButton.Intersect(
                    SummaryLayout.DeployButton));
        TestTrue(
            FString::Printf(
                TEXT("Pause pointer targets remain distinct at %.0fx%.0f"),
                Viewport.X,
                Viewport.Y),
            InsideViewport(PauseLayout.ResumeButton) &&
                InsideViewport(PauseLayout.RestartButton) &&
                InsideViewport(PauseLayout.ReturnButton) &&
                InsideViewport(PauseLayout.PrimaryButton) &&
                !PauseLayout.ResumeButton.Intersect(
                    PauseLayout.RestartButton) &&
                !PauseLayout.RestartButton.Intersect(
                    PauseLayout.ReturnButton) &&
                !PauseLayout.ReturnButton.Intersect(
                    PauseLayout.PrimaryButton));
        TestTrue(
            FString::Printf(
                TEXT("Result pointer targets remain distinct at %.0fx%.0f"),
                Viewport.X,
                Viewport.Y),
            InsideViewport(ResultLayout.FullButton) &&
                InsideViewport(ResultLayout.PrimaryButton) &&
                InsideViewport(ResultLayout.RestartButton) &&
                !ResultLayout.PrimaryButton.Intersect(
                    ResultLayout.RestartButton));
    }

    FEchoesSkirmishSetup InvalidSetup =
        FEchoesSkirmishSetupModel::DefaultSetup();
    FString ValidationError;
    InvalidSetup.OpponentFaction = InvalidSetup.LocalFaction;
    TestFalse(TEXT("A mirror matchup is rejected"),
              FEchoesSkirmishSetupModel::Validate(
                  InvalidSetup, ValidationError));
    TestTrue(TEXT("Mirror rejection is explicit"),
             ValidationError.Contains(TEXT("SKIRMISH_MATCHUP_INVALID")));
    InvalidSetup = FEchoesSkirmishSetupModel::DefaultSetup();
    InvalidSetup.MapPreset = static_cast<EEchoesSkirmishMapPreset>(255);
    TestFalse(TEXT("An unknown battlefield is rejected"),
              FEchoesSkirmishSetupModel::Validate(
                  InvalidSetup, ValidationError));
    InvalidSetup = FEchoesSkirmishSetupModel::DefaultSetup();
    InvalidSetup.AiPersonality =
        static_cast<echoes::sim::AiPersonality>(255);
    TestFalse(TEXT("An unknown AI profile is rejected"),
              FEchoesSkirmishSetupModel::Validate(
                  InvalidSetup, ValidationError));
    InvalidSetup = FEchoesSkirmishSetupModel::DefaultSetup();
    InvalidSetup.ResourceLevel =
        static_cast<EEchoesSkirmishResourceLevel>(255);
    TestFalse(TEXT("An unknown resource profile is rejected"),
              FEchoesSkirmishSetupModel::Validate(
                  InvalidSetup, ValidationError));

    echoes::sim::Simulation AiFixture;
    TestTrue(TEXT("AI fixture accepts local authority"),
             AiFixture.AddPlayer(
                 0,
                 echoes::sim::Faction::MeridianCompact,
                 {500, 30}));
    TestTrue(TEXT("AI fixture accepts opposing authority"),
             AiFixture.AddPlayer(
                 1,
                 echoes::sim::Faction::KharuunAssemblies,
                 {500, 30}));
    const echoes::sim::EntityId AiWorker = AiFixture.SpawnEntity(
        1,
        echoes::sim::Faction::KharuunAssemblies,
        echoes::sim::EntityType::Worker,
        echoes::sim::Vec2::FromTiles(8, 8));
    const echoes::sim::EntityId AiWell =
        AiFixture.SpawnFutureWell(echoes::sim::Vec2::FromTiles(9, 8));
    const std::optional<echoes::sim::PlayerView> AiView =
        AiFixture.CreatePlayerView(1);
    TestTrue(TEXT("AI profile fixture is visible and authoritative"),
             AiWorker != 0 && AiWell != 0 && AiView.has_value());
    if (AiView.has_value())
    {
        const std::vector<echoes::sim::Command> EconomicCommands =
            echoes::sim::Simulation::GenerateAiCommands(
                *AiView, echoes::sim::AiPersonality::Economic);
        const std::vector<echoes::sim::Command> RaiderCommands =
            echoes::sim::Simulation::GenerateAiCommands(
                *AiView, echoes::sim::AiPersonality::Raider);
        const auto FindWellChoice = [](const auto& Commands)
        {
            const auto It = std::find_if(
                Commands.begin(),
                Commands.end(),
                [](const echoes::sim::Command& Command)
                {
                    return Command.type ==
                        echoes::sim::CommandType::FutureWell;
                });
            return It != Commands.end()
                ? It->wellChoice
                : echoes::sim::FutureWellChoice::Dormant;
        };
        TestTrue(
            TEXT("Economic and Raider profiles make materially different Well decisions"),
            FindWellChoice(EconomicCommands) ==
                    echoes::sim::FutureWellChoice::Preserve &&
                FindWellChoice(RaiderCommands) ==
                    echoes::sim::FutureWellChoice::Reshape);
    }

    FTestWorldWrapper WorldWrapper;
    if (!WorldWrapper.CreateTestWorld(EWorldType::Game))
    {
        WorldWrapper.ForwardErrorMessages(this);
        AddError(TEXT("Could not create the skirmish-setup test world."));
        return false;
    }
    UWorld* World = WorldWrapper.GetTestWorld();
    UEchoesSimulationSubsystem* Bridge =
        World != nullptr
            ? World->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    if (!TestNotNull(
            TEXT("Skirmish setup world owns the simulation subsystem"),
            Bridge) ||
        !TestTrue(
            TEXT("Default skirmish starts"),
            Bridge != nullptr && Bridge->StartPrototypeScenario()))
    {
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }
    AEchoesPlayerController* Controller =
        World->SpawnActor<AEchoesPlayerController>();
    AActor* GlassScarCompositionProbe = World->SpawnActor<AActor>();
    if (!TestNotNull(
            TEXT("Skirmish setup controller can be created"),
            Controller) ||
        !TestNotNull(
            TEXT("Battlefield presentation probe can be created"),
            GlassScarCompositionProbe))
    {
        Bridge->StopPrototypeScenario();
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }
    GlassScarCompositionProbe->Tags.Add(
        TEXT("EchoesGlassScarComposition"));

    const echoes::sim::Simulation* DefaultSimulation =
        Bridge->GetSimulation();
    const echoes::sim::Tick DefaultTick =
        DefaultSimulation != nullptr ? DefaultSimulation->CurrentTick() : 0;
    const uint64 DefaultChecksum =
        DefaultSimulation != nullptr ? DefaultSimulation->StateChecksum() : 0;
    const int32 DefaultEntityCount = DefaultSimulation != nullptr
        ? static_cast<int32>(DefaultSimulation->Entities().size())
        : 0;
    const FEchoesSkirmishSetup DefaultSetup =
        Bridge->GetActiveSkirmishSetup();
    Controller->PresentTitleScreen();
    TestTrue(TEXT("Offline title exposes skirmish setup"),
             Controller->IsSkirmishSetupVisible());
    const FVector2D TestViewport(1600.0f, 900.0f);
    const auto BoxCenter = [](const FBox2D& Box)
    {
        return (Box.Min + Box.Max) * 0.5f;
    };
    const FEchoesSkirmishSetupOverlayLayout InitialSetupLayout =
        FEchoesSkirmishSetupOverlayLayout::Build(TestViewport, 1.0f);
    TestTrue(
        TEXT("Pointer increases the selected battlefield row"),
        Controller->HandleModalOverlayPointer(
            BoxCenter(InitialSetupLayout.SettingIncrease[2]),
            TestViewport,
            1.0f) &&
            Controller->GetSkirmishSetupFocusRow() == 2 &&
            Controller->GetPendingSkirmishSetup().MapPreset ==
                EEchoesSkirmishMapPreset::CrownfallBasin);
    TestTrue(
        TEXT("Pointer decreases the same battlefield row"),
        Controller->HandleModalOverlayPointer(
            BoxCenter(InitialSetupLayout.SettingDecrease[2]),
            TestViewport,
            1.0f) &&
            Controller->GetPendingSkirmishSetup().MapPreset ==
                EEchoesSkirmishMapPreset::GlassScar);

    FEchoesSkirmishSetup SetupA = DefaultSetup;
    SetupA.LocalFaction = echoes::sim::Faction::HollowChoir;
    SetupA.OpponentFaction = echoes::sim::Faction::MeridianCompact;
    SetupA.MapPreset = EEchoesSkirmishMapPreset::SorynConfluence;
    SetupA.AiPersonality = echoes::sim::AiPersonality::Raider;
    SetupA.ResourceLevel = EEchoesSkirmishResourceLevel::Abundant;
    FString Feedback;
    TestTrue(TEXT("A complete nondefault setup can be staged"),
             Controller->SetPendingSkirmishSetup(SetupA, Feedback));
    Controller->FocusNextSkirmishSetting();
    Controller->FocusPreviousSkirmishSetting();
    TestTrue(
        TEXT("Setup focus and staging do not mutate the live match"),
        Bridge->GetSimulation() == DefaultSimulation &&
            Bridge->GetSimulation()->CurrentTick() == DefaultTick &&
            Bridge->GetSimulation()->StateChecksum() == DefaultChecksum &&
            static_cast<int32>(Bridge->GetSimulation()->Entities().size()) ==
                DefaultEntityCount &&
            Bridge->GetActiveSkirmishSetup() == DefaultSetup);

    const FEchoesSkirmishSetup PendingBeforeInvalid =
        Controller->GetPendingSkirmishSetup();
    InvalidSetup = PendingBeforeInvalid;
    InvalidSetup.OpponentFaction = InvalidSetup.LocalFaction;
    TestFalse(TEXT("Invalid staged setup is rejected without mutation"),
              Controller->SetPendingSkirmishSetup(
                  InvalidSetup, Feedback));
    TestTrue(TEXT("Rejected staging retains the prior complete draft"),
             Controller->GetPendingSkirmishSetup() ==
                 PendingBeforeInvalid);

    Controller->HandleModalOverlayPointer(
        BoxCenter(InitialSetupLayout.ReviewButton),
        TestViewport,
        1.0f);
    TestTrue(TEXT("First confirmation opens the deployment summary"),
             Controller->IsSkirmishDeploymentSummaryVisible());
    TestTrue(TEXT("Review still does not rebuild the simulation"),
             Bridge->GetSimulation() == DefaultSimulation &&
                 Bridge->GetActiveSkirmishSetup() == DefaultSetup);
    const FEchoesSkirmishSummaryOverlayLayout SummaryLayout =
        FEchoesSkirmishSummaryOverlayLayout::Build(TestViewport, 1.0f);
    Controller->HandleModalOverlayPointer(
        BoxCenter(SummaryLayout.BackButton),
        TestViewport,
        1.0f);
    TestTrue(TEXT("Escape path returns to setup with the draft retained"),
             Controller->IsSkirmishSetupVisible() &&
                 Controller->GetPendingSkirmishSetup() == SetupA);
    Controller->HandleModalOverlayPointer(
        BoxCenter(InitialSetupLayout.ReviewButton),
        TestViewport,
        1.0f);
    Controller->HandleModalOverlayPointer(
        BoxCenter(SummaryLayout.DeployButton),
        TestViewport,
        1.0f);
    TestTrue(TEXT("Second confirmation atomically deploys the setup"),
             !Controller->IsMissionBriefingVisible() &&
                 Bridge->GetActiveSkirmishSetup() == SetupA &&
                 Bridge->GetLocalFaction() == SetupA.LocalFaction &&
                 Bridge->GetOpponentFaction() == SetupA.OpponentFaction);
    TestTrue(TEXT("Alternate battlefield suppresses fixed Glass Scar scenery"),
             GlassScarCompositionProbe->IsHidden());

    const auto CountBlockedTiles = [](const echoes::sim::Simulation* Simulation)
    {
        int32 Count = 0;
        if (Simulation == nullptr)
        {
            return Count;
        }
        for (int32 TileY = 0;
             TileY < FEchoesSkirmishSetupModel::MapHeightTiles;
             ++TileY)
        {
            for (int32 TileX = 0;
                 TileX < FEchoesSkirmishSetupModel::MapWidthTiles;
                 ++TileX)
            {
                Count += Simulation->TerrainAt(TileX, TileY) ==
                    echoes::sim::Terrain::Blocked;
            }
        }
        return Count;
    };
    const auto VerifyRuntimeSetup =
        [this, Bridge, GlassScarCompositionProbe, &CountBlockedTiles,
         &IdentityResearch](
            const FEchoesSkirmishSetup& Setup,
            const TCHAR* Label)
    {
        const echoes::sim::Simulation* Simulation =
            Bridge->GetSimulation();
        if (!TestNotNull(
                FString::Printf(TEXT("%s simulation exists"), Label),
                Simulation))
        {
            return false;
        }
        const echoes::sim::PlayerState* Local = Simulation->FindPlayer(
            UEchoesSimulationSubsystem::LocalPlayerId);
        const echoes::sim::PlayerState* Opponent = Simulation->FindPlayer(
            UEchoesSimulationSubsystem::OpponentPlayerId);
        const echoes::sim::ResourcePool ExpectedResources =
            FEchoesSkirmishSetupModel::StartingResources(
                Setup.ResourceLevel);
        TSet<echoes::sim::EntityType> LocalTypes;
        int32 LocalCount = 0;
        int32 OpponentCount = 0;
        int32 ResourceCount = 0;
        int32 FutureWellCount = 0;
        echoes::sim::Vec2 LocalCorePosition{};
        echoes::sim::Vec2 OpponentCorePosition{};
        echoes::sim::Vec2 FutureWellPosition{};
        bool bLocalFactionsCorrect = true;
        bool bOpponentFactionsCorrect = true;
        for (const echoes::sim::Entity& Entity : Simulation->Entities())
        {
            if (Entity.owner == UEchoesSimulationSubsystem::LocalPlayerId)
            {
                ++LocalCount;
                LocalTypes.Add(Entity.type);
                bLocalFactionsCorrect &= Entity.faction == Setup.LocalFaction;
                if (Entity.type == echoes::sim::EntityType::CommandCore)
                {
                    LocalCorePosition = Entity.position;
                }
            }
            else if (Entity.owner ==
                     UEchoesSimulationSubsystem::OpponentPlayerId)
            {
                ++OpponentCount;
                bOpponentFactionsCorrect &=
                    Entity.faction == Setup.OpponentFaction;
                if (Entity.type == echoes::sim::EntityType::CommandCore)
                {
                    OpponentCorePosition = Entity.position;
                }
            }
            ResourceCount +=
                Entity.type == echoes::sim::EntityType::ResourceNode;
            if (Entity.type == echoes::sim::EntityType::FutureWell)
            {
                ++FutureWellCount;
                FutureWellPosition = Entity.position;
            }
        }
        const echoes::sim::EntityType RequiredTypes[] = {
            echoes::sim::EntityType::Worker,
            echoes::sim::EntityType::Soldier,
            echoes::sim::EntityType::HeavyUnit,
            echoes::sim::EntityType::ScoutUnit,
            echoes::sim::EntityType::CommandCore,
            echoes::sim::EntityType::Dropoff,
            echoes::sim::EntityType::Barracks,
            echoes::sim::EntityType::UtilityStructure};
        bool bCompleteRoster = true;
        for (const echoes::sim::EntityType Type : RequiredTypes)
        {
            bCompleteRoster &= LocalTypes.Contains(Type);
        }
        const int32 FactionIndex = static_cast<int32>(Setup.LocalFaction);
        const echoes::sim::ResearchRules* Research =
            FactionIndex >= 0 &&
                    FactionIndex < UE_ARRAY_COUNT(IdentityResearch)
                ? Simulation->ResearchDefinition(
                      IdentityResearch[FactionIndex])
                : nullptr;
        const TArray<FIntPoint> LocalSpawns =
            FEchoesSkirmishSetupModel::LocalSpawnTiles(Setup.MapPreset);
        const TArray<FIntPoint> OpponentSpawns =
            FEchoesSkirmishSetupModel::OpponentSpawnTiles(Setup.MapPreset);
        const FIntPoint WellTile =
            FEchoesSkirmishSetupModel::FutureWellTile(Setup.MapPreset);
        return TestTrue(
            FString::Printf(TEXT("%s runtime matches its full setup contract"), Label),
            Bridge->GetActiveSkirmishSetup() == Setup &&
                Local != nullptr && Opponent != nullptr &&
                Local->faction == Setup.LocalFaction &&
                Opponent->faction == Setup.OpponentFaction &&
                Local->resources == ExpectedResources &&
                Opponent->resources == ExpectedResources &&
                LocalCount == 12 && OpponentCount == 11 &&
                bLocalFactionsCorrect && bOpponentFactionsCorrect &&
                bCompleteRoster && Research != nullptr &&
                Research->faction == Setup.LocalFaction &&
                ResourceCount == 8 && FutureWellCount == 1 &&
                LocalCorePosition == echoes::sim::Vec2::FromTiles(
                    LocalSpawns[0].X, LocalSpawns[0].Y) &&
                OpponentCorePosition == echoes::sim::Vec2::FromTiles(
                    OpponentSpawns[0].X, OpponentSpawns[0].Y) &&
                FutureWellPosition == echoes::sim::Vec2::FromTiles(
                    WellTile.X, WellTile.Y) &&
                CountBlockedTiles(Simulation) ==
                    FEchoesSkirmishSetupModel::ExpectedBlockedTileCount(
                        Setup.MapPreset) &&
                Bridge->GetTerrainView() != nullptr &&
                Bridge->GetFogView() != nullptr &&
                GlassScarCompositionProbe->IsHidden() ==
                    (Setup.MapPreset !=
                     EEchoesSkirmishMapPreset::GlassScar));
    };

    TestTrue(TEXT("Player-facing Soryn deployment is authoritative"),
             VerifyRuntimeSetup(SetupA, TEXT("Soryn Choir")));
    for (int32 Index = 0; Index < UE_ARRAY_COUNT(Factions); ++Index)
    {
        FEchoesSkirmishSetup Setup;
        Setup.LocalFaction = Factions[Index];
        Setup.OpponentFaction =
            Factions[(Index + 1) % UE_ARRAY_COUNT(Factions)];
        Setup.MapPreset = Maps[Index];
        Setup.AiPersonality = Profiles[Index];
        Setup.ResourceLevel = ResourceLevels[Index];
        Feedback.Reset();
        TestTrue(
            FString::Printf(
                TEXT("%s deployment applies"),
                FEchoesSkirmishSetupModel::FactionDisplayName(
                    Setup.LocalFaction)),
            Bridge->ApplySkirmishSetup(Setup, Feedback));
        TestTrue(
            FString::Printf(
                TEXT("%s keeps its roster, research, map, AI, and resources"),
                FEchoesSkirmishSetupModel::FactionDisplayName(
                    Setup.LocalFaction)),
            VerifyRuntimeSetup(
                Setup,
                FEchoesSkirmishSetupModel::FactionDisplayName(
                    Setup.LocalFaction)));
    }
    TestTrue(TEXT("Faction coverage returns to the selected setup"),
             Bridge->GetActiveSkirmishSetup() == SetupA);

    Feedback.Reset();
    TestTrue(TEXT("A complete skirmish setup is checkpoint-bound"),
             Bridge->QuickSaveScenario(Feedback));
    const echoes::sim::Tick SavedSetupTick =
        Bridge->GetSimulation()->CurrentTick();
    const uint64 SavedSetupChecksum =
        Bridge->GetSimulation()->StateChecksum();
    FEchoesSkirmishSetup SetupB = SetupA;
    SetupB.OpponentFaction = echoes::sim::Faction::KharuunAssemblies;
    SetupB.MapPreset = EEchoesSkirmishMapPreset::CrownfallBasin;
    SetupB.AiPersonality = echoes::sim::AiPersonality::Economic;
    SetupB.ResourceLevel = EEchoesSkirmishResourceLevel::Scarce;
    TestTrue(TEXT("A second complete setup applies"),
             Bridge->ApplySkirmishSetup(SetupB, Feedback));
    const echoes::sim::Simulation* SetupBSimulation =
        Bridge->GetSimulation();
    const echoes::sim::Tick SetupBTick = SetupBSimulation->CurrentTick();
    const uint64 SetupBChecksum = SetupBSimulation->StateChecksum();
    TestTrue(TEXT("A v3 checkpoint restores its explicit full setup"),
             Bridge->QuickLoadScenario(Feedback));
    TestTrue(
        TEXT("Cross-setup recovery replaces all five fields and saved state"),
        Feedback.Contains(TEXT("SORYN CONFLUENCE")) &&
            Bridge->GetActiveSkirmishSetup() == SetupA &&
            Bridge->GetLocalFaction() == SetupA.LocalFaction &&
            Bridge->GetOpponentFaction() == SetupA.OpponentFaction &&
            Bridge->GetSimulation() != SetupBSimulation &&
            Bridge->GetSimulation()->CurrentTick() == SavedSetupTick &&
            Bridge->GetSimulation()->StateChecksum() == SavedSetupChecksum &&
            (SetupBTick != SavedSetupTick ||
             SetupBChecksum != SavedSetupChecksum));

    Bridge->SetScenarioPaused(false);
    Bridge->Tick(0.15f);
    Bridge->SetScenarioPaused(true);
    const echoes::sim::Simulation* BeforeFailedDeployment =
        Bridge->GetSimulation();
    const echoes::sim::Tick BeforeFailedTick =
        BeforeFailedDeployment->CurrentTick();
    const uint64 BeforeFailedChecksum =
        BeforeFailedDeployment->StateChecksum();
    const std::vector<echoes::sim::Entity> BeforeFailedEntities =
        BeforeFailedDeployment->Entities();
    const echoes::sim::ResourcePool BeforeFailedLocalResources =
        BeforeFailedDeployment->FindPlayer(
            UEchoesSimulationSubsystem::LocalPlayerId)->resources;
    AddExpectedError(
        TEXT("ECHOES_SKIRMISH_SETUP_FAILED"),
        EAutomationExpectedErrorFlags::Contains,
        1);
    Bridge->FailNextScenarioStartForTesting();
    TestFalse(TEXT("Injected deployment failure is surfaced"),
              Bridge->ApplySkirmishSetup(SetupB, Feedback));
    TestTrue(
        TEXT("Failed deployment restores the exact prior live match"),
        Feedback.Contains(TEXT("SKIRMISH_DEPLOYMENT_FAILED")) &&
            Bridge->GetActiveSkirmishSetup() == SetupA &&
            Bridge->GetSimulation() == BeforeFailedDeployment &&
            Bridge->GetSimulation()->CurrentTick() == BeforeFailedTick &&
            Bridge->GetSimulation()->StateChecksum() == BeforeFailedChecksum &&
            Bridge->GetSimulation()->Entities() == BeforeFailedEntities &&
            Bridge->GetSimulation()->FindPlayer(
                UEchoesSimulationSubsystem::LocalPlayerId)->resources ==
                BeforeFailedLocalResources &&
            Bridge->IsScenarioReady() && Bridge->IsScenarioPaused() &&
            Bridge->GetTerrainView() != nullptr &&
            Bridge->GetFogView() != nullptr);

    const FEchoesCampaignProgress CampaignBeforeReturn =
        Bridge->GetCampaignProgress();
    Controller->TogglePauseMenu();
    TestTrue(TEXT("Field menu opens on the active skirmish"),
             Controller->IsPauseMenuVisible() &&
                 Bridge->IsScenarioPaused());
    const FEchoesPauseOverlayLayout PauseLayout =
        FEchoesPauseOverlayLayout::Build(TestViewport, 1.0f);
    Controller->HandleModalOverlayPointer(
        BoxCenter(PauseLayout.ResumeButton),
        TestViewport,
        1.0f);
    TestTrue(TEXT("Pause resume is pointer-operable"),
             !Controller->IsPauseMenuVisible() &&
                 !Bridge->IsScenarioPaused());
    Controller->TogglePauseMenu();
    TestTrue(TEXT("Field menu reopens for pointer return"),
             Controller->IsPauseMenuVisible() &&
                 Bridge->IsScenarioPaused());
    const echoes::sim::Simulation* BeforeReturnSimulation =
        Bridge->GetSimulation();
    const echoes::sim::Tick BeforeReturnTick =
        BeforeReturnSimulation->CurrentTick();
    const uint64 BeforeReturnChecksum =
        BeforeReturnSimulation->StateChecksum();
    Controller->HandleModalOverlayPointer(
        BoxCenter(PauseLayout.ReturnButton),
        TestViewport,
        1.0f);
    TestTrue(TEXT("First return request only arms confirmation"),
             Controller->IsReturnToOperationsConfirmationArmed() &&
                 Controller->IsPauseMenuVisible() &&
                 Bridge->GetSimulation() == BeforeReturnSimulation);
    Controller->HandleModalOverlayPointer(
        BoxCenter(PauseLayout.ReturnButton),
        TestViewport,
        1.0f);
    TestTrue(
        TEXT("Confirmed return preserves the paused field and campaign ledger"),
        Controller->IsTitleScreenVisible() &&
            Controller->IsSkirmishSetupVisible() &&
            !Controller->IsPauseMenuVisible() &&
            Bridge->IsScenarioPaused() &&
            Bridge->GetSimulation() == BeforeReturnSimulation &&
            Bridge->GetSimulation()->CurrentTick() == BeforeReturnTick &&
            Bridge->GetSimulation()->StateChecksum() == BeforeReturnChecksum &&
            Bridge->GetCampaignProgress().Decisions ==
                CampaignBeforeReturn.Decisions);
    Controller->ConfirmPrimaryAction();
    Controller->ConfirmPrimaryAction();
    TestTrue(
        TEXT("Unchanged setup resumes the exact paused match"),
        !Controller->IsMissionBriefingVisible() &&
            !Bridge->IsScenarioPaused() &&
            Bridge->GetSimulation() == BeforeReturnSimulation &&
            Bridge->GetSimulation()->CurrentTick() == BeforeReturnTick &&
            Bridge->GetSimulation()->StateChecksum() == BeforeReturnChecksum);

    Controller->PresentTitleScreen();
    const FEchoesSkirmishSetup PendingBeforeNetworkLock =
        Controller->GetPendingSkirmishSetup();
    const echoes::sim::Simulation* BeforeNetworkLock =
        Bridge->GetSimulation();
    const uint64 BeforeNetworkChecksum =
        BeforeNetworkLock->StateChecksum();
    Bridge->SetNetworkHumanOpponent(true);
    TestFalse(TEXT("Network authority suppresses offline skirmish setup"),
              Controller->IsSkirmishSetupVisible());
    TestFalse(TEXT("Network authority rejects local setup application"),
              Bridge->ApplySkirmishSetup(SetupB, Feedback));
    Controller->CyclePlayableFaction();
    TestTrue(
        TEXT("Network setup lock is explicit and nonmutating"),
        Feedback.Contains(TEXT("SKIRMISH_SETUP_NETWORK_LOCKED")) &&
            Controller->GetPendingSkirmishSetup() ==
                PendingBeforeNetworkLock &&
            Bridge->GetSimulation() == BeforeNetworkLock &&
            Bridge->GetSimulation()->StateChecksum() ==
                BeforeNetworkChecksum &&
            Bridge->GetActiveSkirmishSetup() == SetupA);
    Bridge->SetNetworkHumanOpponent(false);

    AEchoesPlayerController* PausePointerController =
        World->SpawnActor<AEchoesPlayerController>();
    if (TestNotNull(
            TEXT("Pause pointer controller can be created"),
            PausePointerController))
    {
        PausePointerController->TogglePauseMenu();
        TestTrue(
            TEXT("Pause restart is pointer-operable"),
            PausePointerController->HandleModalOverlayPointer(
                BoxCenter(PauseLayout.RestartButton),
                TestViewport,
                1.0f) &&
                !PausePointerController->IsPauseMenuVisible() &&
                Bridge->GetMatchOutcome() ==
                    echoes::sim::MatchOutcome::Ongoing);
        PausePointerController->Destroy();
    }

    GlassScarCompositionProbe->Destroy();
    Controller->Destroy();
    Bridge->StopPrototypeScenario();
    WorldWrapper.ForwardErrorMessages(this);

    FTestWorldWrapper FreshWorldWrapper;
    if (!FreshWorldWrapper.CreateTestWorld(EWorldType::Game))
    {
        FreshWorldWrapper.ForwardErrorMessages(this);
        AddError(TEXT("Could not create the cold-start quickload world."));
        return false;
    }
    UWorld* FreshWorld = FreshWorldWrapper.GetTestWorld();
    UEchoesSimulationSubsystem* FreshBridge = FreshWorld != nullptr
        ? FreshWorld->GetSubsystem<UEchoesSimulationSubsystem>()
        : nullptr;
    AActor* FreshGlassScarProbe = FreshWorld != nullptr
        ? FreshWorld->SpawnActor<AActor>()
        : nullptr;
    if (FreshGlassScarProbe != nullptr)
    {
        FreshGlassScarProbe->Tags.Add(TEXT("EchoesGlassScarComposition"));
    }
    Feedback.Reset();
    TestTrue(
        TEXT("A fresh default subsystem starts before alternate quickload"),
        FreshBridge != nullptr && FreshBridge->StartPrototypeScenario() &&
            FreshBridge->GetActiveSkirmishSetup() ==
                FEchoesSkirmishSetupModel::DefaultSetup());
    TestTrue(
        TEXT("Cold-start quickload recovers a nondefault setup transactionally"),
        FreshBridge != nullptr && FreshBridge->QuickLoadScenario(Feedback) &&
            FreshBridge->GetActiveSkirmishSetup() == SetupA &&
            FreshBridge->GetLocalFaction() == SetupA.LocalFaction &&
            FreshBridge->GetOpponentFaction() == SetupA.OpponentFaction &&
            FreshBridge->GetSimulation()->CurrentTick() == SavedSetupTick &&
            FreshBridge->GetSimulation()->StateChecksum() ==
                SavedSetupChecksum &&
            CountBlockedTiles(FreshBridge->GetSimulation()) ==
                FEchoesSkirmishSetupModel::ExpectedBlockedTileCount(
                    SetupA.MapPreset) &&
            FreshGlassScarProbe != nullptr &&
            FreshGlassScarProbe->IsHidden());
    AEchoesPlayerController* FreshController = FreshWorld != nullptr
        ? FreshWorld->SpawnActor<AEchoesPlayerController>()
        : nullptr;
    if (TestNotNull(
            TEXT("Cold-start return controller can be created"),
            FreshController))
    {
        FreshBridge->SetScenarioPaused(false);
        FreshController->TogglePauseMenu();
        FreshController->RequestReturnToOperations();
        FreshController->RequestReturnToOperations();
        TestTrue(
            TEXT("Cold-loaded setup remains selected after returning from the field"),
            FreshController->IsTitleScreenVisible() &&
                FreshController->IsSkirmishSetupVisible() &&
                FreshController->GetPendingSkirmishSetup() == SetupA &&
                FreshBridge->GetActiveSkirmishSetup() == SetupA);
        FreshController->Destroy();
    }
    if (FreshGlassScarProbe != nullptr)
    {
        FreshGlassScarProbe->Destroy();
    }
    if (FreshBridge != nullptr)
    {
        FreshBridge->StopPrototypeScenario();
    }
    FreshWorldWrapper.ForwardErrorMessages(this);
    return !HasAnyErrors() && !WorldWrapper.HasFailed() &&
        !FreshWorldWrapper.HasFailed();
}

#endif

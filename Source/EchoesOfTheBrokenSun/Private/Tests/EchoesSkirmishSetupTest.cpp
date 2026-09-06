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
                echoes::sim::ResourcePool{250, 18} &&
            FEchoesSkirmishSetupModel::StartingResources(ResourceLevels[1]) ==
                echoes::sim::ResourcePool{400, 30} &&
            FEchoesSkirmishSetupModel::StartingResources(ResourceLevels[2]) ==
                echoes::sim::ResourcePool{700, 60});

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
        bool bSetupTargetsSafe = InsideViewport(SetupLayout.ReviewButton) &&
            InsideViewport(SetupLayout.AssistedBannerBox);
        for (int32 Row = 0; Row < 9; ++Row)
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
        bSetupTargetsSafe &=
            !SetupLayout.SettingRows[8].Intersect(SetupLayout.AssistedBannerBox) &&
            !SetupLayout.AssistedBannerBox.Intersect(SetupLayout.ReviewButton);
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

    // The release boundary requires "any of three factions versus any faction,
    // including mirror matchups", so a mirror is a VALID configuration. This
    // block previously asserted the opposite, which is why three of the nine
    // required matchups were unreachable.
    FEchoesSkirmishSetup MirrorSetup =
        FEchoesSkirmishSetupModel::DefaultSetup();
    FString ValidationError;
    MirrorSetup.OpponentFaction = MirrorSetup.LocalFaction;
    TestTrue(TEXT("A mirror matchup is a valid configuration"),
             FEchoesSkirmishSetupModel::Validate(
                 MirrorSetup, ValidationError));

    // And the faction cycler must be able to REACH one: stepping the opponent
    // onto the local faction is a legal stop, not a state to skip past.
    FEchoesSkirmishSetup Cycled = FEchoesSkirmishSetupModel::DefaultSetup();
    bool bReachedMirror = false;
    for (int32 Step = 0; Step < 3; ++Step)
    {
        Cycled = FEchoesSkirmishSetupModel::WithNextFaction(Cycled, false, 1);
        if (Cycled.LocalFaction == Cycled.OpponentFaction)
        {
            bReachedMirror = true;
            break;
        }
    }
    TestTrue(TEXT("Cycling the opponent can reach a mirror matchup"),
             bReachedMirror);

    FEchoesSkirmishSetup InvalidSetup =
        FEchoesSkirmishSetupModel::DefaultSetup();
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

    // The release boundary names exactly five AI doctrines - Warden, Raider,
    // Steward, Expansionist, Adaptive - and section 16.1 documents a purpose, a
    // posture and a Well preference for each. A sixth selectable profile is
    // accessible content with no documented player purpose, which AUTH-005 and
    // VAL-003 make a release blocker. The selector used to offer
    // AiPersonality::Balanced as a sixth "BALANCED" doctrine.
    {
        FEchoesSkirmishSetup DoctrineCycle =
            FEchoesSkirmishSetupModel::DefaultSetup();
        TSet<echoes::sim::AiPersonality> Reachable;
        TSet<FString> DoctrineNames;
        for (int32 Step = 0; Step < 24; ++Step)
        {
            DoctrineCycle =
                FEchoesSkirmishSetupModel::WithNextAi(DoctrineCycle, 1);
            Reachable.Add(DoctrineCycle.AiPersonality);
            DoctrineNames.Add(FString(
                FEchoesSkirmishSetupModel::AiDisplayName(
                    DoctrineCycle.AiPersonality)));
        }
        for (int32 Step = 0; Step < 24; ++Step)
        {
            DoctrineCycle =
                FEchoesSkirmishSetupModel::WithNextAi(DoctrineCycle, -1);
            Reachable.Add(DoctrineCycle.AiPersonality);
        }
        TestEqual(
            TEXT("The doctrine selector offers exactly the five authored doctrines"),
            Reachable.Num(),
            5);
        TestTrue(
            TEXT("The five reachable doctrines are the five the spec authors"),
            Reachable.Contains(echoes::sim::AiPersonality::Defensive) &&
                Reachable.Contains(echoes::sim::AiPersonality::Raider) &&
                Reachable.Contains(echoes::sim::AiPersonality::Economic) &&
                Reachable.Contains(
                    echoes::sim::AiPersonality::Expansionist) &&
                Reachable.Contains(echoes::sim::AiPersonality::Adaptive));
        TestFalse(
            TEXT("The undocumented sixth profile is unreachable from the selector"),
            Reachable.Contains(echoes::sim::AiPersonality::Balanced));
        TestEqual(
            TEXT("Every reachable doctrine carries its own player-facing name"),
            DoctrineNames.Num(),
            5);

        FEchoesSkirmishSetup UnauthoredDoctrine =
            FEchoesSkirmishSetupModel::DefaultSetup();
        UnauthoredDoctrine.AiPersonality =
            echoes::sim::AiPersonality::Balanced;
        TestFalse(
            TEXT("An undocumented AI doctrine is rejected before deployment"),
            FEchoesSkirmishSetupModel::Validate(
                UnauthoredDoctrine, ValidationError));
    }

    // MAP-001 spawn fairness, measured rather than asserted. Both distance
    // clauses are checked from the two anchors that matter: the Command Core,
    // which is what an army or scout leaves from, and the nearer of Command
    // Core or starting Dropoff, which is what "resource travel time" actually
    // means for a hauling worker. Crownfall Basin used to put its Well 35 tiles
    // from one start and 49 from the other.
    {
        const int32 Width = FEchoesSkirmishSetupModel::MapWidthTiles;
        const int32 Height = FEchoesSkirmishSetupModel::MapHeightTiles;
        const auto FloodFill = [Width, Height](
            EEchoesSkirmishMapPreset Preset,
            const FIntPoint& Start)
        {
            TArray<int32> Distance;
            Distance.Init(TNumericLimits<int32>::Max(), Width * Height);
            TArray<FIntPoint> Frontier;
            if (Start.X < 0 || Start.X >= Width || Start.Y < 0 ||
                Start.Y >= Height ||
                FEchoesSkirmishSetupModel::IsBlockedTile(
                    Preset, Start.X, Start.Y))
            {
                return Distance;
            }
            Distance[Start.Y * Width + Start.X] = 0;
            Frontier.Add(Start);
            const FIntPoint Steps[] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
            for (int32 Cursor = 0; Cursor < Frontier.Num(); ++Cursor)
            {
                const FIntPoint Current = Frontier[Cursor];
                const int32 Here = Distance[Current.Y * Width + Current.X];
                for (const FIntPoint& Step : Steps)
                {
                    const FIntPoint Next = Current + Step;
                    if (Next.X < 0 || Next.X >= Width || Next.Y < 0 ||
                        Next.Y >= Height ||
                        FEchoesSkirmishSetupModel::IsBlockedTile(
                            Preset, Next.X, Next.Y))
                    {
                        continue;
                    }
                    int32& Best = Distance[Next.Y * Width + Next.X];
                    if (Best > Here + 1)
                    {
                        Best = Here + 1;
                        Frontier.Add(Next);
                    }
                }
            }
            return Distance;
        };
        // MAP-001's ceiling is 5%, so the two measurements may differ by at
        // most one twentieth of the larger.
        const auto WithinCeiling = [](int32 A, int32 B)
        {
            return A != TNumericLimits<int32>::Max() &&
                B != TNumericLimits<int32>::Max() &&
                FMath::Abs(A - B) * 20 <= FMath::Max(A, B);
        };
        for (int32 MapIndex = 0; MapIndex < UE_ARRAY_COUNT(Maps); ++MapIndex)
        {
            const EEchoesSkirmishMapPreset Preset = Maps[MapIndex];
            const TCHAR* MapName =
                FEchoesSkirmishSetupModel::MapDisplayName(Preset);
            const TArray<FIntPoint> LocalSpawns =
                FEchoesSkirmishSetupModel::LocalSpawnTiles(Preset);
            const TArray<FIntPoint> OpponentSpawns =
                FEchoesSkirmishSetupModel::OpponentSpawnTiles(Preset);
            const TArray<FIntPoint> Deposits =
                FEchoesSkirmishSetupModel::ResourceNodeTiles(Preset);
            const FIntPoint Well =
                FEchoesSkirmishSetupModel::FutureWellTile(Preset);
            if (LocalSpawns.Num() < 3 || OpponentSpawns.Num() < 3 ||
                Deposits.Num() == 0)
            {
                AddError(FString::Printf(
                    TEXT("%s has no measurable deployment contract"), MapName));
                continue;
            }
            const TArray<int32> LocalCore = FloodFill(Preset, LocalSpawns[0]);
            const TArray<int32> OpponentCore =
                FloodFill(Preset, OpponentSpawns[0]);
            const TArray<int32> LocalDrop = FloodFill(Preset, LocalSpawns[2]);
            const TArray<int32> OpponentDrop =
                FloodFill(Preset, OpponentSpawns[2]);
            const auto At = [Width](const TArray<int32>& Field,
                                    const FIntPoint& Tile)
            {
                return Field[Tile.Y * Width + Tile.X];
            };
            const auto Haul = [&At](const TArray<int32>& Core,
                                    const TArray<int32>& Drop,
                                    const FIntPoint& Tile)
            {
                return FMath::Min(At(Core, Tile), At(Drop, Tile));
            };

            TestTrue(
                FString::Printf(
                    TEXT("%s: Well approach time is equivalent for both forces (%d vs %d tiles)"),
                    MapName,
                    At(LocalCore, Well),
                    At(OpponentCore, Well)),
                WithinCeiling(At(LocalCore, Well), At(OpponentCore, Well)));
            TestTrue(
                FString::Printf(
                    TEXT("%s: Well approach is equivalent from the starting Dropoff too"),
                    MapName),
                WithinCeiling(At(LocalDrop, Well), At(OpponentDrop, Well)));
            TestTrue(
                FString::Printf(
                    TEXT("%s: both forces start with equivalent build area"),
                    MapName),
                WithinCeiling(
                    At(LocalCore, LocalSpawns[2]),
                    At(OpponentCore, OpponentSpawns[2])));

            // Rank the deposits by distance for each force and compare the
            // ladders rank for rank: the nth-nearest deposit must cost each
            // force the same travel, or one force simply opens richer.
            TArray<int32> LocalCoreLadder;
            TArray<int32> OpponentCoreLadder;
            TArray<int32> LocalHaulLadder;
            TArray<int32> OpponentHaulLadder;
            for (const FIntPoint& Deposit : Deposits)
            {
                LocalCoreLadder.Add(At(LocalCore, Deposit));
                OpponentCoreLadder.Add(At(OpponentCore, Deposit));
                LocalHaulLadder.Add(Haul(LocalCore, LocalDrop, Deposit));
                OpponentHaulLadder.Add(
                    Haul(OpponentCore, OpponentDrop, Deposit));
            }
            LocalCoreLadder.Sort();
            OpponentCoreLadder.Sort();
            LocalHaulLadder.Sort();
            OpponentHaulLadder.Sort();
            bool bCoreLaddersFair = true;
            bool bHaulLaddersFair = true;
            for (int32 Rank = 0; Rank < Deposits.Num(); ++Rank)
            {
                bCoreLaddersFair &= WithinCeiling(
                    LocalCoreLadder[Rank], OpponentCoreLadder[Rank]);
                bHaulLaddersFair &= WithinCeiling(
                    LocalHaulLadder[Rank], OpponentHaulLadder[Rank]);
            }
            TestTrue(
                FString::Printf(
                    TEXT("%s: every deposit rank is equally far from either Command Core"),
                    MapName),
                bCoreLaddersFair);
            TestTrue(
                FString::Printf(
                    TEXT("%s: worker haul time is equivalent at every deposit rank"),
                    MapName),
                bHaulLaddersFair);
        }
    }

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
        TEXT("Pointer increases the selected teams row"),
        Controller->HandleModalOverlayPointer(
            BoxCenter(InitialSetupLayout.SettingIncrease[2]),
            TestViewport,
            1.0f) &&
            Controller->GetSkirmishSetupFocusRow() == 2 &&
            Controller->GetPendingSkirmishSetup().TeamSetup ==
                EEchoesSkirmishTeamSetup::FreeForAll);
    TestTrue(
        TEXT("Pointer decreases the same teams row"),
        Controller->HandleModalOverlayPointer(
            BoxCenter(InitialSetupLayout.SettingDecrease[2]),
            TestViewport,
            1.0f) &&
            Controller->GetPendingSkirmishSetup().TeamSetup ==
                EEchoesSkirmishTeamSetup::OneVsOne);
    TestTrue(
        TEXT("Pointer increases the selected battlefield row"),
        Controller->HandleModalOverlayPointer(
            BoxCenter(InitialSetupLayout.SettingIncrease[3]),
            TestViewport,
            1.0f) &&
            Controller->GetSkirmishSetupFocusRow() == 3 &&
            Controller->GetPendingSkirmishSetup().MapPreset ==
                EEchoesSkirmishMapPreset::CrownfallBasin);
    TestTrue(
        TEXT("Pointer decreases the same battlefield row"),
        Controller->HandleModalOverlayPointer(
            BoxCenter(InitialSetupLayout.SettingDecrease[3]),
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
    // A mirror matchup used to serve as this block's invalid example. Mirrors
    // are now a REQUIRED configuration, so the rejection this block exists to
    // prove needs a setup that is still genuinely invalid: an unauthored map.
    InvalidSetup.MapPreset = static_cast<EEchoesSkirmishMapPreset>(255);
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
    // The preceding failed-deployment fixture deliberately retained a paused
    // match. Resume it before testing the ordinary running -> menu -> running route.
    Bridge->SetScenarioPaused(false);
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
    Controller->ToggleTacticalPause();
    const auto TacticalPauseTick = Bridge->GetSimulation()->CurrentTick();
    Bridge->Tick(0.5f);
    TestEqual(TEXT("Tactical pause freezes the simulation accumulator"), Bridge->GetSimulation()->CurrentTick(), TacticalPauseTick);
    Controller->TogglePauseMenu();
    Controller->HandleModalOverlayPointer(BoxCenter(PauseLayout.ResumeButton), TestViewport, 1.0f);
    TestTrue(TEXT("Returning from menu preserves an active tactical pause"),
        !Controller->IsPauseMenuVisible() && Bridge->IsScenarioPaused());
    Controller->ToggleTacticalPause();
    TestFalse(TEXT("Tactical pause can be independently resumed"), Bridge->IsScenarioPaused());
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
    // --- F1 Option-by-Option Tests & Mirror Matchup Matrix ---
    {
        // 1. Local Faction Option
        FEchoesSkirmishSetup LocalTestSetup = FEchoesSkirmishSetupModel::DefaultSetup();
        const echoes::sim::Faction InitialLocal = LocalTestSetup.LocalFaction;
        LocalTestSetup = FEchoesSkirmishSetupModel::WithNextFaction(LocalTestSetup, true, 1);
        TestTrue(TEXT("Option 1 (Local Faction): Cycler changes local force"),
                 LocalTestSetup.LocalFaction != InitialLocal);
        TestEqual(TEXT("Option 1 (Local Faction): Displays correct name"),
                  FString(FEchoesSkirmishSetupModel::FactionDisplayName(LocalTestSetup.LocalFaction)),
                  FString(TEXT("KHARUUN ASSEMBLIES")));

        // 2. Opponent Faction Option & 9 Matchups Matrix (including mirror matches)
        const echoes::sim::Faction AllFactions[] = {
            echoes::sim::Faction::MeridianCompact,
            echoes::sim::Faction::KharuunAssemblies,
            echoes::sim::Faction::HollowChoir};
        int32 ValidMatchupCount = 0;
        for (const echoes::sim::Faction LocalFac : AllFactions)
        {
            for (const echoes::sim::Faction OppFac : AllFactions)
            {
                FEchoesSkirmishSetup MatchupSetup = FEchoesSkirmishSetupModel::DefaultSetup();
                MatchupSetup.LocalFaction = LocalFac;
                MatchupSetup.OpponentFaction = OppFac;
                FString MatchupError;
                const bool bValidMatchup = FEchoesSkirmishSetupModel::Validate(MatchupSetup, MatchupError);
                if (bValidMatchup)
                {
                    ++ValidMatchupCount;
                }
                TestTrue(
                    FString::Printf(TEXT("Option 2 (Opponent Faction): %s vs %s is valid (mirror=%s)"),
                        FEchoesSkirmishSetupModel::FactionDisplayName(LocalFac),
                        FEchoesSkirmishSetupModel::FactionDisplayName(OppFac),
                        LocalFac == OppFac ? TEXT("true") : TEXT("false")),
                    bValidMatchup);
            }
        }
        TestEqual(TEXT("Option 2 (Opponent Faction): All 9 matchup combinations are valid"),
                  ValidMatchupCount, 9);

        // 3. Teams Option
        FEchoesSkirmishSetup TeamTestSetup = FEchoesSkirmishSetupModel::DefaultSetup();
        TestEqual(TEXT("Option 3 (Teams): Default is 1v1"),
                  TeamTestSetup.TeamSetup, EEchoesSkirmishTeamSetup::OneVsOne);
        TeamTestSetup = FEchoesSkirmishSetupModel::WithNextTeam(TeamTestSetup, 1);
        TestEqual(TEXT("Option 3 (Teams): Cycles to Free-for-all"),
                  TeamTestSetup.TeamSetup, EEchoesSkirmishTeamSetup::FreeForAll);
        TestEqual(TEXT("Option 3 (Teams): Displays FFA name"),
                  FString(FEchoesSkirmishSetupModel::TeamSetupDisplayName(TeamTestSetup.TeamSetup)),
                  FString(TEXT("FREE-FOR-ALL // INDEPENDENT FORCES")));
        TeamTestSetup = FEchoesSkirmishSetupModel::WithNextTeam(TeamTestSetup, 1);
        TestEqual(TEXT("Option 3 (Teams): Wraps back to 1v1"),
                  TeamTestSetup.TeamSetup, EEchoesSkirmishTeamSetup::OneVsOne);
        TeamTestSetup = FEchoesSkirmishSetupModel::WithNextTeam(TeamTestSetup, -1);
        TestEqual(TEXT("Option 3 (Teams): Cycles backward to Free-for-all"),
                  TeamTestSetup.TeamSetup, EEchoesSkirmishTeamSetup::FreeForAll);

        // 4. Map Preset Option
        FEchoesSkirmishSetup MapTestSetup = FEchoesSkirmishSetupModel::DefaultSetup();
        const EEchoesSkirmishMapPreset InitialMap = MapTestSetup.MapPreset;
        MapTestSetup = FEchoesSkirmishSetupModel::WithNextMap(MapTestSetup, 1);
        TestTrue(TEXT("Option 4 (Battlefield): Cycler changes map preset"),
                 MapTestSetup.MapPreset != InitialMap);
        TestEqual(TEXT("Option 4 (Battlefield): Cycles to Crownfall Basin"),
                  MapTestSetup.MapPreset, EEchoesSkirmishMapPreset::CrownfallBasin);

        // 5. AI Personality Option
        FEchoesSkirmishSetup AiTestSetup = FEchoesSkirmishSetupModel::DefaultSetup();
        const echoes::sim::AiPersonality InitialAi = AiTestSetup.AiPersonality;
        AiTestSetup = FEchoesSkirmishSetupModel::WithNextAi(AiTestSetup, 1);
        TestTrue(TEXT("Option 5 (AI Profile): Cycler changes AI personality"),
                 AiTestSetup.AiPersonality != InitialAi);

        // 6. Difficulty Option & Assisted Handicap Disclosure
        FEchoesSkirmishSetup DiffTestSetup = FEchoesSkirmishSetupModel::DefaultSetup();
        TestEqual(TEXT("Option 6 (Difficulty): Default is Standard"),
                  DiffTestSetup.Difficulty, EEchoesSkirmishDifficulty::Standard);
        DiffTestSetup = FEchoesSkirmishSetupModel::WithNextDifficulty(DiffTestSetup, -1);
        TestEqual(TEXT("Option 6 (Difficulty): Cycles backward to Story"),
                  DiffTestSetup.Difficulty, EEchoesSkirmishDifficulty::Assisted);
        TestEqual(TEXT("Option 6 (Difficulty): Story equal-rules disclosure"),
                  FString(FEchoesSkirmishSetupModel::AssistedDifficultyModifiers()),
                  FString(FEchoesSkirmishSetupModel::DifficultyDescription(EEchoesSkirmishDifficulty::Story)));
        DiffTestSetup = FEchoesSkirmishSetupModel::WithNextDifficulty(DiffTestSetup, 2);
        TestEqual(TEXT("Option 6 (Difficulty): Cycles forward to Veteran"),
                  DiffTestSetup.Difficulty, EEchoesSkirmishDifficulty::Challenging);
        DiffTestSetup = FEchoesSkirmishSetupModel::WithNextDifficulty(DiffTestSetup, 1);
        TestEqual(TEXT("Option 6 (Difficulty): Cycles forward to Sovereign"),
                  DiffTestSetup.Difficulty, EEchoesSkirmishDifficulty::Sovereign);

        // 7. Starting Resources Option
        FEchoesSkirmishSetup ResTestSetup = FEchoesSkirmishSetupModel::DefaultSetup();
        const EEchoesSkirmishResourceLevel InitialRes = ResTestSetup.ResourceLevel;
        ResTestSetup = FEchoesSkirmishSetupModel::WithNextResources(ResTestSetup, 1);
        TestTrue(TEXT("Option 7 (Starting Resources): Cycler changes resource level"),
                 ResTestSetup.ResourceLevel != InitialRes);
        TestEqual(TEXT("Option 7 (Starting Resources): Cycles to Abundant"),
                  ResTestSetup.ResourceLevel, EEchoesSkirmishResourceLevel::Abundant);

        // 8. Victory Condition Option
        FEchoesSkirmishSetup VicTestSetup = FEchoesSkirmishSetupModel::DefaultSetup();
        TestEqual(TEXT("Option 8 (Victory Condition): Default is Corefall"),
                  VicTestSetup.VictoryCondition, EEchoesSkirmishVictoryCondition::Corefall);
        for (int32 Direction : {-1, 1})
        {
            VicTestSetup = FEchoesSkirmishSetupModel::WithNextVictoryCondition(VicTestSetup, Direction);
            TestTrue(TEXT("Offline selector retains Corefall in both directions"), VicTestSetup.VictoryCondition == EEchoesSkirmishVictoryCondition::Corefall);
        }
        for (const auto Unsupported : {EEchoesSkirmishVictoryCondition::WellControl, EEchoesSkirmishVictoryCondition::Conquest})
        {
            VicTestSetup.VictoryCondition = Unsupported;
            FString UnsupportedRuleError;
            TestFalse(TEXT("Legacy unimplemented victory rules are rejected"), FEchoesSkirmishSetupModel::Validate(VicTestSetup, UnsupportedRuleError));
            TestTrue(TEXT("Refusal names actionable Corefall remedy"), UnsupportedRuleError.Contains(TEXT("Corefall")));
        }

        // 9. Game Speed Option & Deterministic Multipliers
        FEchoesSkirmishSetup SpeedTestSetup = FEchoesSkirmishSetupModel::DefaultSetup();
        TestEqual(TEXT("Option 9 (Game Speed): Default is Normal (1.0x)"),
                  SpeedTestSetup.GameSpeed, EEchoesSkirmishGameSpeed::Normal);
        TestEqual(TEXT("Option 9 (Game Speed): Normal multiplier is 1.0f"),
                  FEchoesSkirmishSetupModel::GameSpeedMultiplier(EEchoesSkirmishGameSpeed::Normal),
                  1.0f);
        SpeedTestSetup = FEchoesSkirmishSetupModel::WithNextGameSpeed(SpeedTestSetup, -1);
        TestEqual(TEXT("Option 9 (Game Speed): Cycles backward to Tactical"),
                  SpeedTestSetup.GameSpeed, EEchoesSkirmishGameSpeed::Tactical);
        TestEqual(TEXT("Option 9 (Game Speed): Tactical multiplier is 0.75f"),
                  FEchoesSkirmishSetupModel::GameSpeedMultiplier(EEchoesSkirmishGameSpeed::Tactical),
                  0.75f);
        SpeedTestSetup = FEchoesSkirmishSetupModel::WithNextGameSpeed(SpeedTestSetup, 2);
        TestEqual(TEXT("Option 9 (Game Speed): Cycles forward to Fast"),
                  SpeedTestSetup.GameSpeed, EEchoesSkirmishGameSpeed::Fast);
        TestEqual(TEXT("Option 9 (Game Speed): Fast multiplier is 1.50f"),
                  FEchoesSkirmishSetupModel::GameSpeedMultiplier(EEchoesSkirmishGameSpeed::Fast),
                  1.50f);

        // Validation rejection of invalid enums
        FEchoesSkirmishSetup BadEnumSetup = FEchoesSkirmishSetupModel::DefaultSetup();
        FString BadEnumError;
        BadEnumSetup.TeamSetup = static_cast<EEchoesSkirmishTeamSetup>(255);
        TestFalse(TEXT("Validation rejects invalid team setup"),
                  FEchoesSkirmishSetupModel::Validate(BadEnumSetup, BadEnumError));
        BadEnumSetup = FEchoesSkirmishSetupModel::DefaultSetup();
        BadEnumSetup.Difficulty = static_cast<EEchoesSkirmishDifficulty>(255);
        TestFalse(TEXT("Validation rejects invalid difficulty"),
                  FEchoesSkirmishSetupModel::Validate(BadEnumSetup, BadEnumError));
        BadEnumSetup = FEchoesSkirmishSetupModel::DefaultSetup();
        BadEnumSetup.VictoryCondition = static_cast<EEchoesSkirmishVictoryCondition>(255);
        TestFalse(TEXT("Validation rejects invalid victory condition"),
                  FEchoesSkirmishSetupModel::Validate(BadEnumSetup, BadEnumError));
        BadEnumSetup = FEchoesSkirmishSetupModel::DefaultSetup();
        BadEnumSetup.GameSpeed = static_cast<EEchoesSkirmishGameSpeed>(255);
        TestFalse(TEXT("Validation rejects invalid game speed"),
                  FEchoesSkirmishSetupModel::Validate(BadEnumSetup, BadEnumError));

        // Subsystem game speed multiplier query
        if (FreshBridge != nullptr)
        {
            FEchoesSkirmishSetup SpeedApplySetup = FEchoesSkirmishSetupModel::DefaultSetup();
            SpeedApplySetup.GameSpeed = EEchoesSkirmishGameSpeed::Fast;
            FString SpeedFeedback;
            FreshBridge->ApplySkirmishSetup(SpeedApplySetup, SpeedFeedback);
            TestEqual(TEXT("Simulation subsystem reflects Fast game speed multiplier"),
                      FreshBridge->GetEffectiveGameSpeedMultiplier(),
                      1.50f);
            TestEqual(TEXT("Simulation subsystem reflects active difficulty"),
                      FreshBridge->GetActiveSkirmishDifficulty(),
                      EEchoesSkirmishDifficulty::Standard);
            TestEqual(TEXT("Simulation subsystem reflects active victory condition"),
                      FreshBridge->GetActiveSkirmishVictoryCondition(),
                      EEchoesSkirmishVictoryCondition::Corefall);
            TestEqual(TEXT("Simulation subsystem reflects active team setup"),
                      FreshBridge->GetActiveSkirmishTeamSetup(),
                      EEchoesSkirmishTeamSetup::OneVsOne);

            // Apply Assisted difficulty
            SpeedApplySetup.Difficulty = EEchoesSkirmishDifficulty::Assisted;
            FreshBridge->ApplySkirmishSetup(SpeedApplySetup, SpeedFeedback);
            TestEqual(TEXT("Simulation subsystem reflects active Assisted difficulty"),
                      FreshBridge->GetActiveSkirmishDifficulty(),
                      EEchoesSkirmishDifficulty::Assisted);
        }
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

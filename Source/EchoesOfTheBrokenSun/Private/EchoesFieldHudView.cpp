// Copyright Echoes of the Broken Sun. All Rights Reserved.
// Author: Angelis Pseftis

#include "EchoesFieldHudView.h"

#include "EchoesCampaignRewards.h"
#include "EchoesCinematicSubsystem.h"
#include "Engine/World.h"
#include "EchoesContactIndicatorLayout.h"
#include "EchoesFactionPolicy.h"
#include "EchoesGameInstance.h"
#include "EchoesGameUserSettings.h"
#include "EchoesNarrativeSubsystem.h"
#include "EchoesPlayerController.h"
#include "EchoesRTSCameraPawn.h"
#include "EchoesSimulationSubsystem.h"

#define LOCTEXT_NAMESPACE "EchoesFieldHud"

namespace
{
// Resolve on presentation so remapping never leaves stale default key hints.
FText BoundTutorialText(const FText& Pattern)
{
    return FText::FromString(UEchoesNarrativeSubsystem::ResolveInputTokens(Pattern.ToString()));
}
}


namespace
{
using namespace echoes::sim;

FText Text(const FString& Value)
{
    return FText::FromString(Value);
}

FText EntityName(EntityType Type)
{
    switch (Type)
    {
        case EntityType::Worker: return LOCTEXT("EntityWorker", "Worker");
        case EntityType::Soldier: return LOCTEXT("EntityLineUnit", "Line Unit");
        case EntityType::HeavyUnit: return LOCTEXT("EntityHeavyUnit", "Heavy Unit");
        case EntityType::ScoutUnit: return LOCTEXT("EntityScoutUnit", "Scout Unit");
        case EntityType::CommandCore: return LOCTEXT("EntityCommandCore", "Command Core");
        case EntityType::Barracks: return LOCTEXT("EntityProduction", "Production Structure");
        case EntityType::Dropoff: return LOCTEXT("EntityDropoff", "Resource Drop-off");
        case EntityType::UtilityStructure: return LOCTEXT("EntityUtility", "Utility Structure");
        case EntityType::ResourceNode: return LOCTEXT("EntityMatterNode", "Matter Node");
        case EntityType::FutureWell: return LOCTEXT("EntityFutureWell", "Future Well");
    }
    return LOCTEXT("EntityUnknown", "Entity");
}

FText EntityPurpose(Faction FactionValue, EntityType Type)
{
    if (FactionValue == Faction::MeridianCompact &&
        Type == EntityType::Worker)
    {
        return LOCTEXT("SurveyorPurpose",
            "Core economic builder and logistics conduit. Gathers Matter, constructs Compact structures, operates Future Wells, and provides baseline maintenance. Keep its routes short and scouted: it is unarmed, and route pressure is the opponent's counterplay.");
    }
    return FText::GetEmpty();
}

FText OrderName(OrderType Type)
{
    switch (Type)
    {
        case OrderType::None: return LOCTEXT("OrderIdle", "Idle");
        case OrderType::Move: return LOCTEXT("OrderMoving", "Moving");
        case OrderType::AttackMove: return LOCTEXT("OrderAttackMove", "Attack-move");
        case OrderType::Patrol: return LOCTEXT("OrderPatrol", "Patrolling");
        case OrderType::Hold: return LOCTEXT("OrderHold", "Holding");
        case OrderType::Guard: return LOCTEXT("OrderGuard", "Guarding");
        case OrderType::Attack: return LOCTEXT("OrderAttack", "Attacking");
        case OrderType::Gather: return LOCTEXT("OrderGather", "Gathering");
        case OrderType::Deliver: return LOCTEXT("OrderDeliver", "Delivering");
        case OrderType::Build: return LOCTEXT("OrderBuild", "Building");
        case OrderType::FutureWell: return LOCTEXT("OrderFutureWell", "Committing protocol");
    }
    return LOCTEXT("OrderActive", "Active");
}

FText ResearchName(ResearchType Type)
{
    if (Type == ResearchType::None)
    {
        return LOCTEXT("ResearchNone", "NONE");
    }
    const auto Profiles = {
        echoes::presentation::TechnologyProfile(Faction::MeridianCompact),
        echoes::presentation::TechnologyProfile(Faction::KharuunAssemblies),
        echoes::presentation::TechnologyProfile(Faction::HollowChoir)};
    for (const auto& Profile : Profiles)
    {
        const TCHAR* Id = Type == Profile.TierOne ? Profile.TierOneContentId
            : Type == Profile.TierTwo ? Profile.TierTwoContentId : nullptr;
        if (Id != nullptr)
        {
            return Text(FString(Id).Replace(TEXT("_"), TEXT(" ")).ToUpper());
        }
    }
    return LOCTEXT("ResearchUnknown", "UNKNOWN RESEARCH");
}

FText ResearchStatus(const PlayerView& View)
{
    const PlayerState& Player = View.Player();
    if (Player.activeResearch != ResearchType::None)
    {
        const int32 Percent = FMath::Clamp(
            Player.researchProgress * 100 /
                FMath::Max(1, Player.researchRequired), 0, 100);
        return FText::Format(LOCTEXT("ResearchProgress", "RESEARCH {0} // {1}%"),
            ResearchName(Player.activeResearch), FText::AsNumber(Percent));
    }
    if (Player.lastInterruptedResearch != ResearchType::None)
    {
        return FText::Format(LOCTEXT("ResearchInterrupted", "RESEARCH INTERRUPTED // {0} // NO REFUND"),
            ResearchName(Player.lastInterruptedResearch));
    }
    const auto Profile = echoes::presentation::TechnologyProfile(Player.faction);
    const int32 Completed =
        (Player.HasCompletedResearch(Profile.TierOne) ? 1 : 0) +
        (Player.HasCompletedResearch(Profile.TierTwo) ? 1 : 0);
    return FText::Format(LOCTEXT("ResearchCompleteCount", "RESEARCH {0}/2 COMPLETE"),
        FText::AsNumber(Completed));
}

FVector2D Normalize(Vec2 Position, int32 Width, int32 Height)
{
    return FVector2D(
        FMath::Clamp(static_cast<float>(Position.x.Raw()) /
                         static_cast<float>(FMath::Max(1, Width * kFixedScale)),
                     0.0f, 1.0f),
        FMath::Clamp(static_cast<float>(Position.y.Raw()) /
                         static_cast<float>(FMath::Max(1, Height * kFixedScale)),
                     0.0f, 1.0f));
}

EEchoesFieldHudTileState TileState(
    Visibility VisibilityValue,
    Terrain TerrainValue,
    bool bPassable)
{
    if (VisibilityValue == Visibility::Unexplored)
    {
        return EEchoesFieldHudTileState::Unexplored;
    }
    const bool bVisible = VisibilityValue == Visibility::Visible;
    if (TerrainValue == Terrain::Scarred)
    {
        return bVisible ? EEchoesFieldHudTileState::VisibleScarred
                        : EEchoesFieldHudTileState::ExploredScarred;
    }
    const bool bOpen = TerrainValue == Terrain::Open && bPassable;
    if (bVisible)
    {
        return bOpen ? EEchoesFieldHudTileState::VisibleOpen
                     : EEchoesFieldHudTileState::VisibleBlocked;
    }
    return bOpen ? EEchoesFieldHudTileState::ExploredOpen
                 : EEchoesFieldHudTileState::ExploredBlocked;
}

void AddMarker(
    FEchoesFieldHudMinimapView& Minimap,
    EntityId Id,
    PlayerId Owner,
    Faction FactionValue,
    EntityType Type,
    Vec2 Position,
    PlayerId Viewer,
    bool bRemembered)
{
    FEchoesFieldHudMapMarker Marker;
    Marker.EntityId = Id;
    Marker.NormalizedPosition = Normalize(Position, Minimap.Width, Minimap.Height);
    Marker.Label = EntityName(Type);
    Marker.Owner = Owner;
    Marker.Faction = static_cast<uint8>(FactionValue);
    Marker.EntityType = static_cast<uint8>(Type);
    Marker.bFriendly = Owner == Viewer;
    Marker.bRemembered = bRemembered;
    Marker.bResource = Type == EntityType::ResourceNode;
    Marker.bFutureWell = Type == EntityType::FutureWell;
    Minimap.Markers.Add(MoveTemp(Marker));
}

void AddContacts(
    FEchoesFieldHudMinimapView& Minimap,
    const std::vector<VibrationSignature>& Signatures)
{
    int32 Index = 0;
    for (const VibrationSignature& Signature : Signatures)
    {
        FEchoesFieldHudContact Contact;
        Contact.NormalizedMapPosition =
            Normalize(Signature.approximatePosition, Minimap.Width, Minimap.Height);
        Contact.PrimaryLabel = FText::Format(
            LOCTEXT("ContactOrdinal", "VIBRATION CONTACT {0}"),
            FText::AsNumber(++Index));
        Contact.SecondaryLabel = LOCTEXT("ContactAnonymous", "APPROXIMATE // NO UNIT ID");
        Minimap.Contacts.Add(MoveTemp(Contact));
    }
}

void AddTelegraph(
    FEchoesFieldHudMinimapView& Minimap,
    const FutureWellTelegraph& Telegraph,
    PlayerId /*Viewer*/)
{
    FEchoesFieldHudMapMarker Marker;
    Marker.EntityId = Telegraph.wellId;
    Marker.NormalizedPosition = Normalize(
        Telegraph.position, Minimap.Width, Minimap.Height);
    Marker.Label = LOCTEXT("FutureWellWarning", "Future Well warning");
    Marker.Owner = kNeutralPlayer;
    Marker.EntityType = static_cast<uint8>(EntityType::FutureWell);
    Marker.bFriendly = false;
    Marker.bFutureWell = true;
    Marker.bTelegraphed = true;
    Marker.TelegraphRemainingTicks = Telegraph.remainingTicks;
    Marker.FutureWellChoice = static_cast<uint8>(Telegraph.choice);
    Minimap.Markers.Add(MoveTemp(Marker));
}

void BuildPlayerMinimap(const PlayerView& PlayerView, FEchoesFieldHudMinimapView& Out)
{
    Out.bVisible = true;
    Out.Width = PlayerView.Config().mapWidthTiles;
    Out.Height = PlayerView.Config().mapHeightTiles;
    Out.Tiles.Reserve(Out.Width * Out.Height);
    for (int32 Y = 0; Y < Out.Height; ++Y)
    {
        for (int32 X = 0; X < Out.Width; ++X)
        {
            const Vec2 Tile = Vec2::FromTiles(X, Y);
            const Visibility Seen = PlayerView.VisibilityAt(Tile);
            const Terrain Ground = PlayerView.TerrainAt(X, Y);
            Out.Tiles.Add(TileState(Seen, Ground, PlayerView.IsPositionPassable(Tile)));
        }
    }
    for (const Entity& Entity : PlayerView.Entities())
    {
        AddMarker(Out, Entity.id, Entity.owner, Entity.faction, Entity.type,
                  Entity.position, PlayerView.Player().id, false);
    }
    for (const RememberedObject& Memory : PlayerView.RememberedObjects())
    {
        AddMarker(Out, Memory.id, Memory.owner, Memory.faction, Memory.type,
                  Memory.position, PlayerView.Player().id, true);
    }
    for (const FutureWellTelegraph& Telegraph :
         PlayerView.PublicFutureWellTelegraphs())
    {
        AddTelegraph(Out, Telegraph, PlayerView.Player().id);
    }
    AddContacts(Out, PlayerView.VibrationSignatures());
}

void BuildNetworkMinimap(
    const echoes::sim::net::ScopedViewKeyframe& Keyframe,
    FEchoesFieldHudMinimapView& Out)
{
    Out.bVisible = true;
    Out.Width = Keyframe.mapWidthTiles;
    Out.Height = Keyframe.mapHeightTiles;
    Out.Tiles.Reserve(static_cast<int32>(Keyframe.tiles.size()));
    for (const echoes::sim::net::ScopedTileState& Tile : Keyframe.tiles)
    {
        Out.Tiles.Add(TileState(Tile.visibility, Tile.terrain, Tile.passable));
    }
    for (const echoes::sim::net::ScopedEntityState& Entity : Keyframe.entities)
    {
        AddMarker(Out, Entity.id, Entity.owner, Entity.faction, Entity.type,
                  Entity.position, Keyframe.player, false);
    }
    AddContacts(Out, Keyframe.vibrationSignatures);
}

void AddSelectionEntry(
    const Entity& Entity,
    PlayerId Viewer,
    FEchoesFieldHudSelectionView& Out)
{
    FEchoesFieldHudSelectionEntry Entry;
    Entry.EntityId = Entity.id;
    Entry.Name = Entity.faction == Faction::MeridianCompact &&
        Entity.type == EntityType::Worker
        ? LOCTEXT("EntitySurveyor", "Surveyor")
        : EntityName(Entity.type);
    Entry.Faction = Text(echoes::presentation::FactionDisplayName(Entity.faction));
    Entry.Purpose = EntityPurpose(Entity.faction, Entity.type);
    Entry.Order = OrderName(Entity.order.type);
    Entry.HitPoints = Entity.hitPoints;
    Entry.MaxHitPoints = Entity.maxHitPoints;
    Entry.Cargo = Entity.cargo;
    Entry.CargoCapacity = Entity.cargoCapacity;
    Entry.Damage = Entity.attackDamage;
    if (Entity.productionRequired > 0)
    {
        Entry.Production = EntityName(Entity.productionType);
        Entry.ProductionPercent = FMath::Clamp(
            Entity.productionProgress * 100 /
                FMath::Max(1, Entity.productionRequired), 0, 100);
    }
    Entry.bOwned = Entity.owner == Viewer;
    Entry.bStructure = Entity.type == EntityType::CommandCore ||
        Entity.type == EntityType::Barracks ||
        Entity.type == EntityType::Dropoff ||
        Entity.type == EntityType::UtilityStructure;
    Out.Entries.Add(MoveTemp(Entry));
}

const Entity* FindVisibleEntity(const PlayerView& View, uint32 Id)
{
    for (const Entity& Entity : View.Entities())
    {
        if (Entity.id == Id)
        {
            return &Entity;
        }
    }
    return nullptr;
}

void AddProductionControl(
    FEchoesFieldHudProductionView& Out,
    const FText& Label,
    const FText& Detail,
    EEchoesFieldHudAction Action,
    int32 Slot,
    bool bEnabled = true)
{
    FEchoesFieldHudControl Control;
    Control.Label = Label;
    Control.Detail = Detail;
    Control.Action = Action;
    Control.Argument = Slot;
    Control.bEnabled = bEnabled;
    Out.Controls.Add(MoveTemp(Control));
}

void BuildProductionQueue(
    const PlayerView& PlayerView,
    const TArray<uint32>& SelectedEntityIds,
    FEchoesFieldHudProductionView& Out)
{
    if (SelectedEntityIds.Num() != 1)
    {
        return;
    }
    const EntityId ProducerId = SelectedEntityIds[0];
    const Entity* Producer = FindVisibleEntity(PlayerView, ProducerId);
    if (Producer == nullptr || Producer->owner != PlayerView.Player().id)
    {
        return;
    }

    const ProducerQueueState* Queue = nullptr;
    for (const ProducerQueueState& Candidate : PlayerView.ProducerQueues())
    {
        if (Candidate.producer == ProducerId)
        {
            Queue = &Candidate;
            break;
        }
    }
    if (Queue == nullptr)
    {
        return;
    }

    Out.bVisible = true;
    Out.ProducerId = ProducerId;
    Out.bSpawnBlocked = Queue->spawnBlockedAlert;
    Out.bRallyNeedsAttention = Queue->rallyAlert;
    Out.RallyWaypointCount = static_cast<int32>(Queue->rallyRoute.size());

    if (Queue->active)
    {
        FEchoesFieldHudProductionItem Item;
        Item.Slot = 0;
        Item.ItemId = Queue->activeItem.itemId;
        Item.Unit = EntityName(Queue->activeItem.unitType);
        Item.ProgressPercent = static_cast<int32>(FMath::Clamp<int64>(
            static_cast<int64>(Queue->activeProgress) * 100 /
                FMath::Max(1, Queue->activeItem.requiredTicks),
            0,
            100));
        Item.RequiredTicks = Queue->activeItem.requiredTicks;
        Item.ConfiguredMatter = Queue->activeItem.configuredCost.material;
        Item.ConfiguredDawn = Queue->activeItem.configuredCost.dawnshards;
        Item.InvestedMatter = Queue->activeItem.investedCost.material;
        Item.InvestedDawn = Queue->activeItem.investedCost.dawnshards;
        Item.Logistics = Queue->activeItem.logisticsCost;
        Item.bActive = true;
        Out.Items.Add(Item);

        const int32 RefundPercent =
            static_cast<int64>(Queue->activeProgress) * 2 <
                    Queue->activeItem.requiredTicks
                ? 75
                : 50;
        const int32 RefundMatter = static_cast<int32>(
            static_cast<int64>(Item.InvestedMatter) * RefundPercent / 100);
        const int32 RefundDawn = static_cast<int32>(
            static_cast<int64>(Item.InvestedDawn) * RefundPercent / 100);
        AddProductionControl(
            Out,
            LOCTEXT("CancelActiveProduction", "REVIEW ACTIVE CANCELLATION"),
            FText::Format(
                LOCTEXT("CancelActiveRefund", "Refund {0} Matter / {1} Dawn"),
                FText::AsNumber(RefundMatter),
                FText::AsNumber(RefundDawn)),
            EEchoesFieldHudAction::ProductionCancel,
            0);
    }

    const int32 WaitingCount = static_cast<int32>(Queue->waiting.size());
    for (int32 Index = 0; Index < WaitingCount; ++Index)
    {
        const ProductionQueueItem& Waiting = Queue->waiting[Index];
        const int32 Slot = Index + 1;
        FEchoesFieldHudProductionItem Item;
        Item.Slot = Slot;
        Item.ItemId = Waiting.itemId;
        Item.Unit = EntityName(Waiting.unitType);
        Item.RequiredTicks = Waiting.requiredTicks;
        Item.ConfiguredMatter = Waiting.configuredCost.material;
        Item.ConfiguredDawn = Waiting.configuredCost.dawnshards;
        Item.InvestedMatter = Waiting.investedCost.material;
        Item.InvestedDawn = Waiting.investedCost.dawnshards;
        Item.Logistics = Waiting.logisticsCost;
        Out.Items.Add(Item);

        const FText SlotLabel = FText::AsNumber(Slot);
        AddProductionControl(
            Out,
            FText::Format(
                LOCTEXT("CancelWaitingProduction", "REVIEW CANCEL {0}"),
                SlotLabel),
            LOCTEXT("CancelWaitingUnpaid", "Refund 0 Matter / 0 Dawn"),
            EEchoesFieldHudAction::ProductionCancel,
            Slot);
        AddProductionControl(
            Out,
            FText::Format(
                LOCTEXT("MoveWaitingProductionUp", "MOVE {0} UP"),
                SlotLabel),
            LOCTEXT("WaitingSlotsOnly", "Waiting slots only"),
            EEchoesFieldHudAction::ProductionMoveUp,
            Slot,
            Slot > 1);
        AddProductionControl(
            Out,
            FText::Format(
                LOCTEXT("MoveWaitingProductionDown", "MOVE {0} DOWN"),
                SlotLabel),
            LOCTEXT("WaitingSlotsOnly", "Waiting slots only"),
            EEchoesFieldHudAction::ProductionMoveDown,
            Slot,
            Slot < WaitingCount);
    }
}

const echoes::sim::net::ScopedEntityState* FindScopedEntity(
    const echoes::sim::net::ScopedViewKeyframe& View,
    uint32 Id)
{
    for (const echoes::sim::net::ScopedEntityState& Entity : View.entities)
    {
        if (Entity.id == Id)
        {
            return &Entity;
        }
    }
    return nullptr;
}

FText OutcomeText(MatchOutcome Outcome)
{
    switch (Outcome)
    {
        case MatchOutcome::Player0Victory: return LOCTEXT("OutcomeVictory", "VICTORY");
        case MatchOutcome::Player1Victory:
        case MatchOutcome::Player2Victory:
        case MatchOutcome::Player3Victory: return LOCTEXT("OutcomeDefeat", "DEFEAT");
        case MatchOutcome::Draw: return LOCTEXT("OutcomeDraw", "DRAW");
        case MatchOutcome::Ongoing: break;
    }
    return LOCTEXT("OutcomeActive", "ACTIVE");
}

void AddObjectiveLine(
    FEchoesFieldHudView& View,
    const FText& Label,
    bool bComplete,
    bool bFailed = false)
{
    View.ObjectiveLines.Add({
        Label,
        bFailed ? LOCTEXT("ObjectiveFailed", "FAILED")
                : bComplete ? LOCTEXT("ObjectiveComplete", "COMPLETE")
                            : LOCTEXT("ObjectiveInProgress", "IN PROGRESS"),
        bFailed ? EEchoesFieldHudTone::Danger
                : bComplete ? EEchoesFieldHudTone::Success
                            : EEchoesFieldHudTone::Normal});
}

void AddObjectiveStateLine(
    FEchoesFieldHudView& View,
    const FText& Label,
    const FText& State,
    EEchoesFieldHudTone Tone)
{
    View.ObjectiveLines.Add({Label, State, Tone});
}

void BuildObjectiveView(
    const FEchoesObjectiveSnapshot& Objective,
    FEchoesFieldHudView& View)
{
    View.bObjectiveVisible = Objective.bScenarioReady;
    View.Objective = Objective;
    if (!Objective.bScenarioReady)
    {
        return;
    }
    const bool bFailed = !Objective.bLocalCoreIntact ||
        (Objective.Outcome != MatchOutcome::Ongoing &&
         Objective.Outcome != MatchOutcome::Player0Victory);
    switch (Objective.OperationMode)
    {
        case EEchoesOperationMode::TrainingReadiness:
            View.ObjectiveTitle = LOCTEXT("TrainingObjective", "READINESS CHECK");
            AddObjectiveLine(View, LOCTEXT("TrainingCorefall", "COMPLETE THE CHECK, THEN DEFEAT THE OPPOSING CORE"),
                Objective.Outcome == MatchOutcome::Player0Victory, bFailed);
            break;
        case EEchoesOperationMode::CampaignPrologue:
            View.ObjectiveTitle = LOCTEXT("ObjectiveM01", "WHAT THE LEDGER KEEPS // MISSION 01");
            AddObjectiveLine(View, LOCTEXT("M01Archive", "ARCHIVE CARRIER"),
                Objective.bArchiveCarrierIntact &&
                    Objective.ProloguePhase != EEchoesProloguePhase::RecoverArchive,
                !Objective.bArchiveCarrierIntact);
            AddObjectiveLine(View, LOCTEXT("M01Well", "FUTURE WELL"),
                Objective.PrologueWellChoice != FutureWellChoice::Dormant,
                Objective.bPrologueWellEnemyControlled || Objective.bPrologueReshapeExpired);
            AddObjectiveLine(View, LOCTEXT("M01Withdrawal", "WITHDRAWAL TO LUME REACH"),
                Objective.ProloguePhase == EEchoesProloguePhase::Complete, bFailed);
            break;
        case EEchoesOperationMode::CampaignSevenAccounts:
            View.ObjectiveTitle = LOCTEXT("ObjectiveM02", "SEVEN ACCOUNTS OF RAIN // MISSION 02");
            AddObjectiveLine(View, LOCTEXT("M02Waystone", "WAYSTONE ANCHOR"), Objective.bWaystoneRootedAtAnchor, !Objective.bWaystoneIntact);
            AddObjectiveLine(View, LOCTEXT("M02Memory", "MEMORY ACCOUNT"), Objective.bMemoryBearerAtAccountSite, !Objective.bMemoryBearerIntact);
            AddObjectiveLine(View, LOCTEXT("M02Decision", "INHERITED WELL DECISION"), Objective.SevenAccountsBranch != FutureWellChoice::Dormant, bFailed);
            break;
        case EEchoesOperationMode::CampaignCityReserve:
            View.ObjectiveTitle = LOCTEXT("ObjectiveM03", "A CITY ON RESERVE // MISSION 03");
            AddObjectiveLine(View, LOCTEXT("M03Life", "LIFE SUPPORT DISTRICT"), Objective.bLifeSupportPowered, bFailed);
            AddObjectiveLine(View, LOCTEXT("M03Transit", "TRANSIT DISTRICT"), Objective.bTransitPowered, bFailed);
            AddObjectiveLine(View, LOCTEXT("M03Archive", "ARCHIVE DISTRICT"), Objective.bArchivePowered, bFailed);
            break;
        case EEchoesOperationMode::CampaignUnburiedRoad:
            View.ObjectiveTitle = LOCTEXT("ObjectiveM04", "THE UNBURIED ROAD // MISSION 04");
            AddObjectiveLine(View, LOCTEXT("M04Roadhead", "WAYSTONE ROADHEAD"), Objective.bWaystoneRootedAtRoadhead, bFailed);
            AddObjectiveLine(View, LOCTEXT("M04Spine", "LISTENING SPINE"), Objective.bListeningSpineComplete, bFailed);
            AddObjectiveLine(View, LOCTEXT("M04Shard", "MEMORY SHARD"), Objective.bMemoryBearerAtShard, bFailed);
            break;
        case EEchoesOperationMode::CampaignTermsOfContinuance:
            View.ObjectiveTitle = LOCTEXT("ObjectiveM05", "TERMS OF CONTINUANCE // MISSION 05");
            AddObjectiveLine(View, LOCTEXT("M05Links", "MERIDIAN RELAY + KHARUUN SPINE"), Objective.bMeridianRelaySynchronized && Objective.bKharuunSpineSynchronized, bFailed);
            AddObjectiveLine(View, LOCTEXT("M05Window", "CONTINUANCE WINDOW"), Objective.bContinuanceWindowHeld, bFailed);
            AddObjectiveLine(View, LOCTEXT("M05Witnesses", "WITNESS EXTRACTION"), Objective.bMeridianWitnessExtracted && Objective.bKharuunWitnessExtracted, bFailed);
            break;
        case EEchoesOperationMode::CampaignNamesWithoutBirths:
            View.ObjectiveTitle = LOCTEXT("ObjectiveM06", "NAMES WITHOUT BIRTHS // MISSION 06");
            AddObjectiveLine(View, LOCTEXT("M06Evidence", "CENSUS EVIDENCE"), Objective.bCensusEvidenceLocated, bFailed);
            AddObjectiveLine(View, LOCTEXT("M06Archive", "CENSUS ARCHIVE POWER"), Objective.bCensusArchivePowered, bFailed);
            AddObjectiveLine(View, LOCTEXT("M06Shelter", "CIVILIANS + TALAR EXTRACTION"), Objective.bFirstCivilianSheltered && Objective.bSecondCivilianSheltered && Objective.bTalarAtEvidenceExtraction, bFailed);
            break;
        case EEchoesOperationMode::CampaignShapeOfSilence:
            View.ObjectiveTitle = LOCTEXT("ObjectiveM07", "THE SHAPE OF SILENCE // MISSION 07");
            AddObjectiveLine(View, LOCTEXT("M07Anchor", "WAYSTONE + LISTENING SPINE"), Objective.bShapeWaystoneRooted && Objective.bShapeListeningSpineRaised, bFailed);
            AddObjectiveLine(View, LOCTEXT("M07Witnesses", "MEMORY WITNESSES"), Objective.bFirstMemoryWitnessPositioned && Objective.bSecondMemoryWitnessPositioned, bFailed);
            AddObjectiveLine(View, LOCTEXT("M07Confluence", "ORUUN AT CONFLUENCE"), Objective.bOruunAtConfluence, bFailed);
            break;
        case EEchoesOperationMode::CampaignShapeBesideUs:
            View.ObjectiveTitle = LOCTEXT("ObjectiveM08", "THE SHAPE BESIDE US // MISSION 08");
            AddObjectiveLine(View, LOCTEXT("M08Echo", "FIRST ECHO"), Objective.bFirstEchoObserved, bFailed);
            AddObjectiveLine(View, LOCTEXT("M08Relay", "ECHO RELAY"), Objective.bEchoRelayRaised, bFailed);
            AddObjectiveLine(View, LOCTEXT("M08Convergence", "PAIRED CONVERGENCE"), Objective.bFirstStateTraversed && Objective.bSecondStateTraversed && Objective.bShapeBesideUsTalarAtConvergence, bFailed);
            break;
        case EEchoesOperationMode::CampaignReserveAuthority:
            View.ObjectiveTitle = LOCTEXT("ObjectiveM09", "RESERVE AUTHORITY // MISSION 09");
            AddObjectiveLine(View, LOCTEXT("M09Site", "AUTHORITY SITE"), Objective.bReserveAuthoritySecured, bFailed);
            AddObjectiveLine(View, LOCTEXT("M09Allocation", "DISTRICT RESERVE"), Objective.ReserveAuthorityBranch != FutureWellChoice::Dormant, bFailed);
            AddObjectiveLine(View, LOCTEXT("M09Deferred", "DEFERRED DISTRICT"), Objective.bReserveAuthorityMaraAtDeferredDistrict, bFailed);
            break;
        case EEchoesOperationMode::CampaignChoirAtLumeReach:
            View.ObjectiveTitle = LOCTEXT("ObjectiveM10", "THE CHOIR AT LUME REACH // MISSION 10");
            AddObjectiveLine(View, LOCTEXT("M10Contact", "CONTACT + LIABILITY"), Objective.bChoirContactEstablished && Objective.bChoirDeferredLiabilityResolved, bFailed);
            AddObjectiveLine(View, LOCTEXT("M10Anchors", "LISTENING ANCHORS"), Objective.bChoirFirstAnchorRaised && Objective.bChoirSecondAnchorRaised, bFailed);
            AddObjectiveLine(View, LOCTEXT("M10Resolution", "WELL + RESOLUTION"), Objective.bChoirBranchResolutionCompleted, Objective.bChoirReshapeWindowExpired || bFailed);
            break;
        case EEchoesOperationMode::CampaignNoNeutralLedger:
            View.ObjectiveTitle = LOCTEXT("ObjectiveM11", "NO NEUTRAL LEDGER // MISSION 11");
            AddObjectiveLine(View, LOCTEXT("M11Route", "ROUTE + DISTRICTS"), Objective.bNoNeutralRouteSecured && Objective.bNoNeutralFirstDistrictIntegrated && Objective.bNoNeutralSecondDistrictIntegrated, bFailed);
            AddObjectiveLine(View, LOCTEXT("M11Evidence", "EVIDENCE CHANNELS"), Objective.bNoNeutralPublicInterfacesIntact && Objective.bNoNeutralEvidenceAttested, bFailed);
            AddObjectiveLine(View, LOCTEXT("M11Protocol", "PROTOCOL + COALITION"), Objective.bNoNeutralProtocolApplied && Objective.bNoNeutralCoalitionRallied, Objective.bNoNeutralReshapeWindowExpired || bFailed);
            break;
        case EEchoesOperationMode::CampaignFutureThatWon:
            View.ObjectiveTitle = LOCTEXT("ObjectiveM12", "THE FUTURE THAT WON // MISSION 12");
            AddObjectiveLine(View, LOCTEXT("M12Inputs", "RECORDED INPUTS"), Objective.bFutureWonFirstInputVerified && Objective.bFutureWonSecondInputVerified, bFailed);
            AddObjectiveLine(View, LOCTEXT("M12Readback", "PUBLIC READBACK"), Objective.bFutureWonPublicInterfacesIntact && Objective.bFutureWonIndependentReadbackEstablished && Objective.bFutureWonProtocolBound, bFailed);
            AddObjectiveLine(View, LOCTEXT("M12Stability", "STABILITY + OBSERVATION"), Objective.bFutureWonStabilityWindowHeld && Objective.bFutureWonFirstDistrictReadbackObserved && Objective.bFutureWonSecondDistrictReadbackObserved, Objective.bFutureWonReshapeWindowExpired || bFailed);
            break;
        case EEchoesOperationMode::CampaignAssemblyOfTheMissing:
            View.ObjectiveTitle = LOCTEXT("ObjectiveM13", "ASSEMBLY OF THE MISSING // MISSION 13");
            AddObjectiveLine(View, LOCTEXT("M13Readback", "PUBLIC RECORD READBACK"), Objective.bAssemblyPublicInterfacesIntact && Objective.bAssemblyPublicRecordReadbackEstablished, bFailed);
            AddObjectiveLine(View, LOCTEXT("M13Index", "CROWNFALL INDEX"), Objective.bAssemblyCrownfallIndexLinked, bFailed);
            AddObjectiveLine(View, LOCTEXT("M13Observe", "INDEPENDENT OBSERVATION"), Objective.bAssemblyMeridianWitnessObserved && Objective.bAssemblyKharuunWitnessObserved, bFailed);
            break;
        case EEchoesOperationMode::CampaignSeveralVoicesOneCommand:
            View.ObjectiveTitle = LOCTEXT("ObjectiveM14", "SEVERAL VOICES, ONE COMMAND // MISSION 14");
            AddObjectiveLine(View, LOCTEXT("M14Research", "SHARED RESOLUTION RESEARCH"), Objective.bSeveralVoicesHeldAlternativesResearched && Objective.bSeveralVoicesSharedResolutionResearched, bFailed);
            AddObjectiveLine(View, LOCTEXT("M14Voices", "VOICES AT COMMAND SITE"), Objective.bSeveralVoicesPossibleAtSite && Objective.bSeveralVoicesManifestAtSite && Objective.bSeveralVoicesNemeAtCommandSite, bFailed);
            AddObjectiveLine(View, LOCTEXT("M14Crisis", "CRISIS WINDOW + PHASE ANCHOR"), Objective.bSeveralVoicesCrisisWindowHeld && Objective.bSeveralVoicesPhaseAnchorComplete, bFailed);
            break;
        case EEchoesOperationMode::CampaignTheBrokenSun:
            View.ObjectiveTitle = LOCTEXT("ObjectiveM15", "THE BROKEN SUN // MISSION 15");
            AddObjectiveLine(View, LOCTEXT("M15Approach", "APPROACH"), Objective.bBrokenSunApproachSecured, bFailed);
            AddObjectiveLine(View, LOCTEXT("M15Accord", "THREE-FACTION ACCORD"), Objective.bBrokenSunMeridianAccordEstablished && Objective.bBrokenSunKharuunAccordEstablished && Objective.bBrokenSunChoirAccordEstablished, bFailed);
            AddObjectiveLine(View, LOCTEXT("M15Resolution", "FINAL RESOLUTION"), Objective.bBrokenSunResolutionConduitComplete && Objective.bBrokenSunResolutionWindowHeld, Objective.bBrokenSunResolutionContractFailed || bFailed);
            break;
        case EEchoesOperationMode::Skirmish:
            View.ObjectiveTitle = LOCTEXT("ObjectiveSkirmish", "SKIRMISH // OBJECTIVES");
            AddObjectiveLine(View, LOCTEXT("SkirmishWell", "FUTURE WELL"), Objective.VisibleFutureWellChoice != FutureWellChoice::Dormant, false);
            AddObjectiveLine(View, LOCTEXT("SkirmishCore", "COMMAND CORE"), Objective.bLocalCoreIntact, !Objective.bLocalCoreIntact);
            AddObjectiveStateLine(
                View, LOCTEXT("SkirmishEnemyCore", "OPPOSING COMMAND CORE"),
                Objective.Outcome == MatchOutcome::Player0Victory
                    ? LOCTEXT("SkirmishEnemyCoreDestroyed", "DESTROYED")
                    : Objective.bHostileCoreVisible
                        ? LOCTEXT("SkirmishEnemyCoreVisible", "INTACT // VISIBLE")
                        : LOCTEXT("SkirmishEnemyCoreUnknown", "UNKNOWN // FOG"),
                Objective.Outcome == MatchOutcome::Player0Victory
                    ? EEchoesFieldHudTone::Success
                    : Objective.bHostileCoreVisible
                        ? EEchoesFieldHudTone::Warning
                        : EEchoesFieldHudTone::Muted);
            break;
    }
}

void ApplySettings(const UEchoesGameUserSettings* Settings, FEchoesFieldHudView& View)
{
    if (Settings == nullptr)
    {
        return;
    }
    View.HudScale = Settings->GetHudScale();
    View.bHighContrast = Settings->IsHighContrastHudEnabled();
    View.bReducedMotion = Settings->IsReducedMotionEnabled();
    View.bReducedFlashing = Settings->IsReducedFlashingEnabled();
}

void BuildTechnology(
    const PlayerView& PlayerView,
    const TArray<uint32>& SelectedIds,
    int32 FocusedTier,
    bool bVisible,
    FEchoesFieldHudTechnologyView& Out)
{
    Out.bVisible = bVisible;
    if (!bVisible)
    {
        return;
    }
    const PlayerState& Player = PlayerView.Player();
    Out.Title = FText::Format(LOCTEXT("TechnologyTitle", "{0} // TECHNOLOGY ARCHIVE"),
        Text(echoes::presentation::FactionDisplayName(Player.faction)));
    Out.ActiveResearch = ResearchStatus(PlayerView);
    const auto Profile = echoes::presentation::TechnologyProfile(Player.faction);
    const ResearchType Types[] = {Profile.TierOne, Profile.TierTwo};
    const TCHAR* Names[] = {Profile.TierOneContentId, Profile.TierTwoContentId};
    const Entity* SelectedProducer = nullptr;
    for (uint32 Id : SelectedIds)
    {
        const Entity* Entity = FindVisibleEntity(PlayerView, Id);
        if (Entity != nullptr && Entity->owner == Player.id &&
            Entity->type == EntityType::Barracks)
        {
            SelectedProducer = Entity;
            break;
        }
    }
    const bool bSelectedProducer = SelectedProducer != nullptr;
    const bool bSelectedProducerBusy =
        bSelectedProducer && SelectedProducer->productionRequired > 0;
    for (int32 Index = 0; Index < 2; ++Index)
    {
        FEchoesFieldHudTechnologyTier Tier;
        Tier.Tier = Index;
        Tier.Name = Text(FString(Names[Index]).Replace(TEXT("_"), TEXT(" ")).ToUpper());
        Tier.bFocused = Index == FocusedTier;
        const uint8 RuleIndex = static_cast<uint8>(Types[Index]);
        const ResearchRules* Rules = RuleIndex < PlayerView.Config().rules.research.size()
            ? &PlayerView.Config().rules.research[RuleIndex] : nullptr;
        if (Rules != nullptr)
        {
            Tier.Cost = FText::Format(
                LOCTEXT("TechnologyCost", "Matter {0} // Dawn {1} // {2}s"),
                FText::AsNumber(Rules->cost.material),
                FText::AsNumber(Rules->cost.dawnshards),
                FText::AsNumber(
                    static_cast<double>(Rules->researchTicks) /
                        FMath::Max<uint32>(1, PlayerView.Config().ticksPerSecond)));
            Tier.Description = Rules->combatDamagePercent > 100
                ? FText::Format(LOCTEXT("TechnologyDamage", "+{0}% combat damage"),
                    FText::AsNumber(Rules->combatDamagePercent - 100))
                : FText::Format(LOCTEXT("TechnologyVision", "+{0}% combat vision"),
                    FText::AsNumber(Rules->combatVisionPercent - 100));
        }
        const bool bComplete = Player.HasCompletedResearch(Types[Index]);
        const bool bActive = Player.activeResearch == Types[Index];
        const bool bInterrupted = Player.lastInterruptedResearch == Types[Index];
        const bool bPrerequisite = Rules != nullptr &&
            (Rules->prerequisite == ResearchType::None || Player.HasCompletedResearch(Rules->prerequisite));
        const bool bFunded = Rules != nullptr &&
            Player.resources.material >= Rules->cost.material &&
            Player.resources.dawnshards >= Rules->cost.dawnshards;
        if (bComplete)
        {
            Tier.State = LOCTEXT("TechComplete", "COMPLETE");
            Tier.Tone = EEchoesFieldHudTone::Success;
        }
        else if (bActive)
        {
            const int32 Percent = FMath::Clamp(
                Player.researchProgress * 100 / FMath::Max(1, Player.researchRequired), 0, 100);
            Tier.State = FText::Format(LOCTEXT("TechnologyResearching", "RESEARCHING {0}%"),
                FText::AsNumber(Percent));
            Tier.Tone = EEchoesFieldHudTone::Accent;
        }
        else if (bInterrupted)
        {
            Tier.State = LOCTEXT("TechInterrupted", "INTERRUPTED // COSTS LOST");
            Tier.Tone = EEchoesFieldHudTone::Danger;
        }
        else if (Player.activeResearch != ResearchType::None)
        {
            Tier.State = LOCTEXT("TechBusy", "BUSY // ANOTHER PROJECT IS ACTIVE");
            Tier.Tone = EEchoesFieldHudTone::Muted;
        }
        else if (bSelectedProducerBusy)
        {
            Tier.State = LOCTEXT(
                "TechProductionBusy", "BUSY // PRODUCTION IS ACTIVE");
            Tier.Tone = EEchoesFieldHudTone::Muted;
        }
        else if (!bPrerequisite)
        {
            Tier.State = LOCTEXT("TechLocked", "LOCKED // COMPLETE PRIOR TIER");
            Tier.Tone = EEchoesFieldHudTone::Muted;
        }
        else if (!bFunded)
        {
            Tier.State = LOCTEXT("TechInsufficient", "INSUFFICIENT RESOURCES");
            Tier.Tone = EEchoesFieldHudTone::Danger;
        }
        else if (!bSelectedProducer)
        {
            Tier.State = LOCTEXT("TechSelectProducer", "READY // SELECT A PRODUCTION STRUCTURE");
            Tier.Tone = EEchoesFieldHudTone::Warning;
        }
        else
        {
            Tier.State = LOCTEXT("TechReady", "READY // ACTIVATE TO RESEARCH");
            Tier.Tone = EEchoesFieldHudTone::Accent;
            Tier.bEnabled = true;
        }
        Out.Tiers.Add(MoveTemp(Tier));
        FEchoesFieldHudControl TierControl;
        TierControl.Label = Out.Tiers.Last().Name;
        TierControl.Detail = Out.Tiers.Last().State;
        TierControl.Action = EEchoesFieldHudAction::TechnologyResearchTier;
        TierControl.Argument = Index;
        TierControl.bEnabled = Out.Tiers.Last().bEnabled;
        TierControl.bFocused = Out.Tiers.Last().bFocused;
        Out.Controls.Add(MoveTemp(TierControl));
    }
    FEchoesFieldHudControl Previous;
    Previous.Label = LOCTEXT("TechPrevious", "PREVIOUS TIER");
    Previous.Action = EEchoesFieldHudAction::TechnologyPrevious;
    Out.Controls.Add(MoveTemp(Previous));
    FEchoesFieldHudControl Next;
    Next.Label = LOCTEXT("TechNext", "NEXT TIER");
    Next.Action = EEchoesFieldHudAction::TechnologyNext;
    Out.Controls.Add(MoveTemp(Next));
    FEchoesFieldHudControl Close;
    Close.Label = LOCTEXT("TechClose", "CLOSE TECHNOLOGY ARCHIVE");
    Close.Action = EEchoesFieldHudAction::ToggleTechnology;
    Out.Controls.Add(MoveTemp(Close));
}

void BuildCommandControls(
    const FEchoesCommandDeckProfile& Profile,
    FEchoesFieldHudCommandView& Out)
{
    Out.bVisible = Profile.WorkerCount + Profile.CombatCount +
        Profile.StructureCount + Profile.OtherCount > 0;
    for (const FEchoesCommandDeckActionEntry& Entry :
         FEchoesCommandDeckModel::BuildActionEntries(Profile))
    {
        FEchoesFieldHudControl Control;
        Control.Label = Text(Entry.Label);
        Control.Detail = Text(Entry.Hotkey);
        Control.Action = EEchoesFieldHudAction::CommandDeck;
        Control.Argument = static_cast<int32>(Entry.Action);
        Control.bPrimary = Entry.bRequiresCursorTarget;
        Out.Controls.Add(MoveTemp(Control));
    }
}

FEchoesCommandDeckProfile BuildNetworkCommandProfile(
    const echoes::sim::net::ScopedViewKeyframe& Keyframe,
    const TArray<uint32>& SelectedEntityIds)
{
    FEchoesCommandDeckProfile Profile;
    for (uint32 Id : SelectedEntityIds)
    {
        const echoes::sim::net::ScopedEntityState* Entity =
            FindScopedEntity(Keyframe, Id);
        if (Entity == nullptr || Entity->owner != Keyframe.player)
        {
            continue;
        }
        switch (Entity->type)
        {
            case EntityType::Worker:
                ++Profile.WorkerCount;
                break;
            case EntityType::Soldier:
            case EntityType::HeavyUnit:
            case EntityType::ScoutUnit:
                ++Profile.CombatCount;
                break;
            case EntityType::CommandCore:
            case EntityType::Dropoff:
            case EntityType::Barracks:
            case EntityType::UtilityStructure:
                ++Profile.StructureCount;
                Profile.bHasCommandCore |=
                    Entity->type == EntityType::CommandCore;
                Profile.bHasBarracks |= Entity->type == EntityType::Barracks;
                break;
            default:
                ++Profile.OtherCount;
                break;
        }
    }
    return Profile;
}

FVector NetworkWorldPosition(
    const FVector2D& NormalizedPosition,
    const FEchoesFieldHudMinimapView& Minimap)
{
    return FVector(
        (NormalizedPosition.X - 0.5f) * Minimap.Width *
            UEchoesSimulationSubsystem::TileWorldSize,
        (NormalizedPosition.Y - 0.5f) * Minimap.Height *
            UEchoesSimulationSubsystem::TileWorldSize,
        0.0f);
}

FVector2D NormalizeNetworkWorldPosition(
    const FVector& WorldPosition,
    const FEchoesFieldHudMinimapView& Minimap)
{
    return FVector2D(
        FMath::Clamp(
            WorldPosition.X /
                    FMath::Max(1.0f, Minimap.Width *
                        UEchoesSimulationSubsystem::TileWorldSize) +
                0.5f,
            0.0f, 1.0f),
        FMath::Clamp(
            WorldPosition.Y /
                    FMath::Max(1.0f, Minimap.Height *
                        UEchoesSimulationSubsystem::TileWorldSize) +
                0.5f,
            0.0f, 1.0f));
}

FVector2D FallbackContactProjection(
    const AEchoesPlayerController& Controller,
    const FVector& WorldPosition,
    const FVector2D& ViewportSize)
{
    FVector CameraLocation = FVector::ZeroVector;
    FRotator CameraRotation = FRotator::ZeroRotator;
    Controller.GetPlayerViewPoint(CameraLocation, CameraRotation);
    FVector GroundDirection = WorldPosition - CameraLocation;
    GroundDirection.Z = 0.0f;
    FVector CameraForward = CameraRotation.Vector();
    CameraForward.Z = 0.0f;
    if (!CameraForward.Normalize())
    {
        CameraForward = FVector::ForwardVector;
    }
    const FVector CameraRight =
        FVector::CrossProduct(FVector::UpVector, CameraForward).GetSafeNormal();
    FVector2D Direction(
        FVector::DotProduct(GroundDirection, CameraRight),
        -FVector::DotProduct(GroundDirection, CameraForward));
    if (!Direction.Normalize())
    {
        Direction = FVector2D(0.0f, -1.0f);
    }
    return ViewportSize * 0.5f + Direction *
        FMath::Max(ViewportSize.X, ViewportSize.Y) * 2.0f;
}

void AddSpatialPresentation(
    const FEchoesFieldHudBuildContext& Context,
    FEchoesFieldHudView& View)
{
    if (!View.Minimap.bVisible || View.Minimap.Width <= 0 ||
        View.Minimap.Height <= 0 || Context.Controller == nullptr ||
        Context.Simulation == nullptr)
    {
        return;
    }
    AEchoesPlayerController* ProjectionController =
        const_cast<AEchoesPlayerController*>(Context.Controller);
    for (FEchoesFieldHudContact& Contact : View.Minimap.Contacts)
    {
        const Vec2 SimPosition = Vec2::FromRaw(
            FMath::RoundToInt(Contact.NormalizedMapPosition.X *
                View.Minimap.Width * kFixedScale),
            FMath::RoundToInt(Contact.NormalizedMapPosition.Y *
                View.Minimap.Height * kFixedScale));
        FVector WorldPosition =
            View.Authority == EEchoesFieldHudAuthority::NetworkKeyframe
                ? NetworkWorldPosition(
                      Contact.NormalizedMapPosition, View.Minimap)
                : Context.Simulation->SimToWorld(SimPosition);
        WorldPosition.Z = 90.0f;
        FVector2D Projected;
        if (!ProjectionController->ProjectWorldLocationToScreen(
                WorldPosition, Projected, true))
        {
            Projected = FallbackContactProjection(
                *Context.Controller, WorldPosition, Context.ViewportSize);
        }
        const FEchoesContactIndicatorPlacement Placement =
            FEchoesContactIndicatorLayout::Calculate(
                Projected, Context.ViewportSize, View.HudScale);
        Contact.NormalizedScreenPosition = FVector2D(
            Placement.MarkerPosition.X / FMath::Max(1.0f, Context.ViewportSize.X),
            Placement.MarkerPosition.Y / FMath::Max(1.0f, Context.ViewportSize.Y));
        Contact.PrimaryLabel = Text(
            FEchoesContactIndicatorLayout::BuildPrimaryLabel(
                static_cast<int32>(
                    &Contact - View.Minimap.Contacts.GetData()) + 1,
                Placement.bClampedToEdge));
        Contact.bScreenPlacementValid = true;
        Contact.bClampedToScreenEdge = Placement.bClampedToEdge;
    }
    const AEchoesRTSCameraPawn* Camera =
        Cast<AEchoesRTSCameraPawn>(Context.Controller->GetPawn());
    TArray<FVector> Footprint;
    if (Camera != nullptr &&
        Camera->GetBattlefieldFootprint(Context.ViewportSize, Footprint) &&
        Footprint.Num() == 4)
    {
        for (const FVector& Corner : Footprint)
        {
            View.Minimap.CameraFrustum.Add(
                View.Authority == EEchoesFieldHudAuthority::NetworkKeyframe
                    ? NormalizeNetworkWorldPosition(Corner, View.Minimap)
                    : Normalize(Context.Simulation->WorldToSim(Corner),
                          View.Minimap.Width, View.Minimap.Height));
        }
    }
}

void BuildMissionMarkers(
    const UEchoesSimulationSubsystem& Bridge,
    const FEchoesObjectiveSnapshot& Objective,
    FEchoesFieldHudMinimapView& Minimap)
{
    const auto Add = [&Minimap](Vec2 Site, const FText& Label, bool bComplete)
    {
        Minimap.MissionMarkers.Add({
            Normalize(Site, Minimap.Width, Minimap.Height), Label,
            bComplete ? EEchoesFieldHudTone::Success
                      : EEchoesFieldHudTone::Accent});
    };
    switch (Objective.OperationMode)
    {
        case EEchoesOperationMode::TrainingReadiness:
            Add(UEchoesSimulationSubsystem::GetArchiveRecoverySite(), LOCTEXT("MapArchiveFull", "Archive"), Objective.ProloguePhase != EEchoesProloguePhase::RecoverArchive);
            Add(UEchoesSimulationSubsystem::GetEvacuationSite(), LOCTEXT("MapEvacFull", "Evac"), Objective.ProloguePhase == EEchoesProloguePhase::Complete);
            break;
        case EEchoesOperationMode::CampaignPrologue:
            Add(UEchoesSimulationSubsystem::GetArchiveRecoverySite(), LOCTEXT("MapArchive", "A"), Objective.ProloguePhase != EEchoesProloguePhase::RecoverArchive);
            Add(UEchoesSimulationSubsystem::GetEvacuationSite(), LOCTEXT("MapEvac", "E"), Objective.ProloguePhase == EEchoesProloguePhase::Complete);
            break;
        case EEchoesOperationMode::CampaignSevenAccounts:
        {
            const auto Plan = Bridge.GetSevenAccountsRoute();
            Add(Plan.WaystoneAnchor, LOCTEXT("MapWaystone", "W"), Objective.bWaystoneRootedAtAnchor);
            Add(Plan.MemoryAccountSite, LOCTEXT("MapMemory", "M"), Objective.bMemoryBearerAtAccountSite);
            break;
        }
        case EEchoesOperationMode::CampaignCityReserve:
            Add(FEchoesCityReserveMissionModel::SiteForDistrict(EEchoesCityDistrict::LifeSupport), LOCTEXT("MapLife", "L"), Objective.bLifeSupportPowered);
            Add(FEchoesCityReserveMissionModel::SiteForDistrict(EEchoesCityDistrict::Transit), LOCTEXT("MapTransit", "T"), Objective.bTransitPowered);
            Add(FEchoesCityReserveMissionModel::SiteForDistrict(EEchoesCityDistrict::Archive), LOCTEXT("MapDistrictArchive", "A"), Objective.bArchivePowered);
            break;
        case EEchoesOperationMode::CampaignUnburiedRoad:
        {
            const auto Plan = Bridge.GetUnburiedRoadRoute();
            Add(Plan.Roadhead, LOCTEXT("MapRoadhead", "W"), Objective.bWaystoneRootedAtRoadhead);
            Add(Plan.ListeningSpineSite, LOCTEXT("MapSpine", "L"), Objective.bListeningSpineComplete);
            Add(Plan.MemoryShardSite, LOCTEXT("MapShard", "S"), Objective.bMemoryBearerAtShard);
            break;
        }
        case EEchoesOperationMode::CampaignTermsOfContinuance:
        {
            const auto Plan = Bridge.GetTermsOfContinuancePlan();
            for (const Vec2& Site : Plan.PlayerPowerLinkSites)
            {
                Add(Site, LOCTEXT("MapPower", "P"), Objective.bContinuanceWindowHeld);
            }
            Add(Plan.MeridianRelaySite, LOCTEXT("MapRelay", "A"), Objective.bMeridianRelaySynchronized);
            Add(Plan.KharuunSpineSite, LOCTEXT("MapKharuun", "K"), Objective.bKharuunSpineSynchronized);
            Add(Plan.WitnessExtractionSite, LOCTEXT("MapWitnessExit", "E"), Objective.bMeridianWitnessExtracted && Objective.bKharuunWitnessExtracted);
            break;
        }
        case EEchoesOperationMode::CampaignNamesWithoutBirths:
        {
            const auto Plan = Bridge.GetNamesWithoutBirthsPlan();
            Add(Plan.CensusSite, LOCTEXT("MapCensus", "C"), Objective.bCensusEvidenceLocated);
            Add(Plan.PowerLinkSite, LOCTEXT("MapCensusPower", "P"), Objective.bCensusArchivePowered);
            Add(Plan.CivilianShelterSite, LOCTEXT("MapShelter", "S"), Objective.bFirstCivilianSheltered && Objective.bSecondCivilianSheltered);
            Add(Plan.EvidenceExtractionSite, LOCTEXT("MapEvidenceExit", "E"), Objective.bTalarAtEvidenceExtraction);
            break;
        }
        case EEchoesOperationMode::CampaignShapeOfSilence:
        {
            const auto Plan = Bridge.GetShapeOfSilencePlan();
            Add(Plan.WaystoneAnchor, LOCTEXT("MapShapeWaystone", "W"), Objective.bShapeWaystoneRooted);
            Add(Plan.ListeningSpineSite, LOCTEXT("MapShapeSpine", "L"), Objective.bShapeListeningSpineRaised);
            Add(Plan.FirstWitnessSite, LOCTEXT("MapFirst", "1"), Objective.bFirstMemoryWitnessPositioned);
            Add(Plan.SecondWitnessSite, LOCTEXT("MapSecond", "2"), Objective.bSecondMemoryWitnessPositioned);
            Add(Plan.ConfluenceSite, LOCTEXT("MapOruun", "O"), Objective.bOruunAtConfluence);
            break;
        }
        case EEchoesOperationMode::CampaignShapeBesideUs:
        {
            const auto Plan = Bridge.GetShapeBesideUsPlan();
            Add(Plan.FirstEchoSite, LOCTEXT("MapNeme", "N"), Objective.bFirstEchoObserved);
            Add(Plan.EchoRelaySite, LOCTEXT("MapEchoRelay", "R"), Objective.bEchoRelayRaised);
            Add(Plan.FirstStateSite, LOCTEXT("MapStateOne", "1"), Objective.bFirstStateTraversed);
            Add(Plan.SecondStateSite, LOCTEXT("MapStateTwo", "2"), Objective.bSecondStateTraversed);
            Add(Plan.ConvergenceSite, LOCTEXT("MapTalar", "T"), Objective.bShapeBesideUsTalarAtConvergence);
            break;
        }
        case EEchoesOperationMode::CampaignReserveAuthority:
        {
            const auto Plan = Bridge.GetReserveAuthorityPlan();
            Add(Plan.AuthoritySite, LOCTEXT("MapReserve", "R"), Objective.bReserveAuthoritySecured);
            Add(FEchoesCityReserveMissionModel::SiteForDistrict(EEchoesCityDistrict::LifeSupport), LOCTEXT("MapReserveLife", "L"), Objective.bLifeSupportPowered);
            Add(FEchoesCityReserveMissionModel::SiteForDistrict(EEchoesCityDistrict::Transit), LOCTEXT("MapReserveTransit", "T"), Objective.bTransitPowered);
            Add(FEchoesCityReserveMissionModel::SiteForDistrict(EEchoesCityDistrict::Archive), LOCTEXT("MapReserveArchive", "A"), Objective.bArchivePowered);
            break;
        }
        case EEchoesOperationMode::CampaignChoirAtLumeReach:
        {
            const auto Plan = Bridge.GetChoirAtLumeReachPlan();
            Add(Plan.ContactSite, LOCTEXT("MapContact", "C"), Objective.bChoirContactEstablished);
            Add(Plan.LiabilitySite, LOCTEXT("MapLiability", "L"), Objective.bChoirDeferredLiabilityResolved);
            Add(Plan.FirstAnchorSite, LOCTEXT("MapAnchorOne", "1"), Objective.bChoirFirstAnchorRaised);
            Add(Plan.SecondAnchorSite, LOCTEXT("MapAnchorTwo", "2"), Objective.bChoirSecondAnchorRaised);
            Add(Plan.FutureWellSite, LOCTEXT("MapChoirWell", "W"), Objective.ChoirAtLumeReachWellChoice != FutureWellChoice::Dormant);
            if (Objective.ChoirAtLumeReachWellChoice != FutureWellChoice::Dormant)
            {
                Add(FEchoesChoirAtLumeReachMissionModel::ResolutionSiteForChoice(
                        Objective.ChoirAtLumeReachWellChoice),
                    LOCTEXT("MapChoirResolution", "R"),
                    Objective.bChoirBranchResolutionCompleted);
            }
            break;
        }
        case EEchoesOperationMode::CampaignNoNeutralLedger:
        {
            const auto Plan = Bridge.GetNoNeutralLedgerPlan();
            Add(Plan.RouteSite, LOCTEXT("MapRoute", "R"), Objective.bNoNeutralRouteSecured);
            Add(Plan.FirstDistrictSite, LOCTEXT("MapDistrictOne", "1"), Objective.bNoNeutralFirstDistrictIntegrated);
            Add(Plan.SecondDistrictSite, LOCTEXT("MapDistrictTwo", "2"), Objective.bNoNeutralSecondDistrictIntegrated);
            Add(Plan.MeridianEvidenceSite, LOCTEXT("MapMeridian", "M"), Objective.bNoNeutralEvidenceAttested);
            Add(Plan.KharuunEvidenceSite, LOCTEXT("MapLedgerKharuun", "K"), Objective.bNoNeutralEvidenceAttested);
            Add(Plan.FutureWellSite, LOCTEXT("MapLedgerWell", "W"), Objective.bNoNeutralProtocolApplied);
            Add(Plan.RallySite, LOCTEXT("MapCoalition", "C"), Objective.bNoNeutralCoalitionRallied);
            break;
        }
        case EEchoesOperationMode::CampaignFutureThatWon:
        {
            const auto Plan = Bridge.GetFutureThatWonPlan();
            Add(Plan.KharuunReadbackSite, LOCTEXT("MapFutureKharuun", "K"), Objective.bFutureWonIndependentReadbackEstablished);
            Add(Plan.MeridianReadbackSite, LOCTEXT("MapFutureMeridian", "M"), Objective.bFutureWonIndependentReadbackEstablished);
            Add(Plan.FirstDistrictInputSite, LOCTEXT("MapInputOne", "1"), Objective.bFutureWonFirstInputVerified);
            Add(Plan.SecondDistrictInputSite, LOCTEXT("MapInputTwo", "2"), Objective.bFutureWonSecondInputVerified);
            Add(Plan.RestorationDemonstratorSite, LOCTEXT("MapDemonstrator", "D"), Objective.bFutureWonProtocolBound);
            Add(Plan.FutureWellSite, LOCTEXT("MapFutureWell", "W"), Objective.bFutureWonStabilityWindowHeld);
            break;
        }
        case EEchoesOperationMode::CampaignAssemblyOfTheMissing:
        {
            const auto Plan = Bridge.GetAssemblyOfTheMissingPlan();
            Add(Plan.MeridianPublicRecordSite, LOCTEXT("MapAssemblyMeridian", "M"), Objective.bAssemblyPublicRecordReadbackEstablished);
            Add(Plan.KharuunPublicRecordSite, LOCTEXT("MapAssemblyKharuun", "K"), Objective.bAssemblyPublicRecordReadbackEstablished);
            Add(Plan.CrownfallIndexSite, LOCTEXT("MapCrownfall", "C"), Objective.bAssemblyCrownfallIndexLinked);
            Add(Plan.MeridianAssemblyWitnessSite, LOCTEXT("MapAssemblyOne", "1"), Objective.bAssemblyMeridianWitnessObserved);
            Add(Plan.KharuunAssemblyWitnessSite, LOCTEXT("MapAssemblyTwo", "2"), Objective.bAssemblyKharuunWitnessObserved);
            break;
        }
        case EEchoesOperationMode::CampaignSeveralVoicesOneCommand:
        {
            const auto Plan = Bridge.GetSeveralVoicesOneCommandPlan();
            Add(Plan.PossibleVoiceSite, LOCTEXT("MapPossible", "P"), Objective.bSeveralVoicesPossibleAtSite);
            Add(Plan.ManifestVoiceSite, LOCTEXT("MapManifest", "M"), Objective.bSeveralVoicesManifestAtSite);
            Add(Plan.NemeCommandSite, LOCTEXT("MapCommandNeme", "N"), Objective.bSeveralVoicesNemeAtCommandSite);
            Add(Plan.CrisisAnchorSite, LOCTEXT("MapCrisis", "A"), Objective.bSeveralVoicesCrisisWindowHeld);
            break;
        }
        case EEchoesOperationMode::CampaignTheBrokenSun:
        {
            const auto Plan = Bridge.GetBrokenSunPlan();
            Add(Plan.CrownfallApproachSite, LOCTEXT("MapApproach", "A"), Objective.bBrokenSunApproachSecured);
            Add(Plan.MaraAccordSite, LOCTEXT("MapMara", "M"), Objective.bBrokenSunMeridianAccordEstablished);
            Add(Plan.OruunAccordSite, LOCTEXT("MapAccordOruun", "O"), Objective.bBrokenSunKharuunAccordEstablished);
            Add(Plan.NemeAccordSite, LOCTEXT("MapAccordNeme", "N"), Objective.bBrokenSunChoirAccordEstablished);
            Add(Plan.TalarPublicRecordSite, LOCTEXT("MapPublicRecord", "T"), Objective.BrokenSunFinalResolution != EEchoesFinalResolution::None);
            if (Objective.BrokenSunFinalResolution != EEchoesFinalResolution::None)
            {
                Add(FEchoesBrokenSunMissionModel::ResolutionConvergenceSite(
                        Plan, Objective.BrokenSunFinalResolution),
                    LOCTEXT("MapFinalResolution", "F"),
                    Objective.bBrokenSunResolutionWindowHeld);
            }
            break;
        }
        case EEchoesOperationMode::Skirmish:
            break;
    }
}
}

FEchoesFieldHudView FEchoesFieldHudModel::BuildPlayerScoped(
    const echoes::sim::PlayerView& PlayerView,
    const TArray<uint32>& SelectedEntityIds,
    bool bReplay)
{
    FEchoesFieldHudView View;
    View.Authority = bReplay ? EEchoesFieldHudAuthority::ReplayPlayerView
                             : EEchoesFieldHudAuthority::LivePlayerView;
    View.Surface = bReplay ? EEchoesFieldHudSurface::Replay
                           : EEchoesFieldHudSurface::Battlefield;
    BuildPlayerMinimap(PlayerView, View.Minimap);
    if (bReplay)
    {
        return View;
    }
    View.Resources.bVisible = true;
    View.Resources.Matter = PlayerView.Player().resources.material;
    View.Resources.Dawn = PlayerView.Player().resources.dawnshards;
    View.Resources.PopulationUsed = PlayerView.PopulationUsed();
    View.Resources.PopulationCapacity = PlayerView.PopulationCapacity();
    View.Resources.SimulationTick = PlayerView.CurrentTick();
    View.Resources.LocalFaction = Text(
        echoes::presentation::FactionDisplayName(PlayerView.Player().faction));
    View.Resources.ResearchStatus = ResearchStatus(PlayerView);
    for (uint32 Id : SelectedEntityIds)
    {
        if (const echoes::sim::Entity* Entity = FindVisibleEntity(PlayerView, Id))
        {
            AddSelectionEntry(*Entity, PlayerView.Player().id, View.Selection);
        }
    }
    View.Selection.bVisible = !View.Selection.Entries.IsEmpty();
    BuildProductionQueue(PlayerView, SelectedEntityIds, View.Production);
    return View;
}

FEchoesFieldHudView FEchoesFieldHudModel::BuildNetworkScoped(
    const echoes::sim::net::ScopedViewKeyframe& Keyframe,
    const TArray<uint32>& SelectedEntityIds)
{
    FEchoesFieldHudView View;
    View.Authority = EEchoesFieldHudAuthority::NetworkKeyframe;
    View.Surface = EEchoesFieldHudSurface::Battlefield;
    View.Resources.bVisible = true;
    View.Resources.Matter = Keyframe.resources.material;
    View.Resources.Dawn = Keyframe.resources.dawnshards;
    View.Resources.PopulationUsed = Keyframe.populationUsed;
    View.Resources.PopulationCapacity = Keyframe.populationCapacity;
    View.Resources.SimulationTick = Keyframe.simulationTick;
    View.Resources.LocalFaction = Text(
        echoes::presentation::FactionDisplayName(Keyframe.faction));
    View.Resources.MatchState = LOCTEXT("NetworkActive", "NETWORK ACTIVE");
    View.Resources.ResearchStatus = FText::Format(
        LOCTEXT("NetworkScopedEntities", "REMOTE BATTLEFIELD // {0} SCOPED ENTITIES"),
        FText::AsNumber(static_cast<int32>(Keyframe.entities.size())));
    BuildNetworkMinimap(Keyframe, View.Minimap);
    for (uint32 Id : SelectedEntityIds)
    {
        const echoes::sim::net::ScopedEntityState* Entity = FindScopedEntity(Keyframe, Id);
        if (Entity == nullptr)
        {
            continue;
        }
        FEchoesFieldHudSelectionEntry Entry;
        Entry.EntityId = Entity->id;
        Entry.Name = EntityName(Entity->type);
        Entry.Faction = Text(echoes::presentation::FactionDisplayName(Entity->faction));
        Entry.HitPoints = Entity->hitPoints;
        Entry.MaxHitPoints = Entity->maxHitPoints;
        Entry.bOwned = Entity->owner == Keyframe.player;
        View.Selection.Entries.Add(MoveTemp(Entry));
    }
    View.Selection.bVisible = !View.Selection.Entries.IsEmpty();
    BuildCommandControls(
        BuildNetworkCommandProfile(Keyframe, SelectedEntityIds),
        View.Commands);
    return View;
}

FEchoesFieldHudView FEchoesFieldHudModel::BuildReplayObserver(
    const echoes::sim::Simulation& ReplaySimulation)
{
    FEchoesFieldHudView View;
    View.Authority = EEchoesFieldHudAuthority::ReplayObserver;
    View.Surface = EEchoesFieldHudSurface::Replay;
    View.Minimap.bVisible = true;
    View.Minimap.Width = ReplaySimulation.Config().mapWidthTiles;
    View.Minimap.Height = ReplaySimulation.Config().mapHeightTiles;
    View.Minimap.Tiles.Reserve(View.Minimap.Width * View.Minimap.Height);
    for (int32 Y = 0; Y < View.Minimap.Height; ++Y)
    {
        for (int32 X = 0; X < View.Minimap.Width; ++X)
        {
            const echoes::sim::Terrain Ground = ReplaySimulation.TerrainAt(X, Y);
            View.Minimap.Tiles.Add(
                Ground == echoes::sim::Terrain::Open
                    ? EEchoesFieldHudTileState::VisibleOpen
                    : Ground == echoes::sim::Terrain::Scarred
                        ? EEchoesFieldHudTileState::VisibleScarred
                        : EEchoesFieldHudTileState::VisibleBlocked);
        }
    }
    for (const echoes::sim::Entity& Entity : ReplaySimulation.Entities())
    {
        AddMarker(View.Minimap, Entity.id, Entity.owner, Entity.faction,
                  Entity.type, Entity.position, echoes::sim::kNeutralPlayer, false);
    }
    for (const echoes::sim::FutureWellTelegraph& Telegraph :
         ReplaySimulation.PublicFutureWellTelegraphs())
    {
        AddTelegraph(View.Minimap, Telegraph, echoes::sim::kNeutralPlayer);
    }
    return View;
}

bool FEchoesFieldHudModel::Build(
    const FEchoesFieldHudBuildContext& Context,
    FEchoesFieldHudView& OutView,
    FString& OutError)
{
    OutError.Reset();
    OutView = FEchoesFieldHudView{};
    if (Context.Controller == nullptr || Context.Simulation == nullptr)
    {
        OutError = TEXT("[FIELD_HUD_SOURCE_MISSING] Controller and simulation bridge are required.");
        return false;
    }
    const AEchoesPlayerController& Controller = *Context.Controller;
    const UEchoesSimulationSubsystem& Bridge = *Context.Simulation;
    ApplySettings(Context.Settings, OutView);

    // Flow state is authoritative even when unattended tests or widget
    // construction failures mean no shell UObject currently exists.
    const FEchoesPlayerFlow& Flow = Controller.GetPlayerFlow();
    const bool bFieldOwnedSurface = Controller.IsCampaignOperationsMapVisible() ||
        Controller.IsOnlineFrontDoorVisible() ||
        Controller.IsOnlineLocalMenuVisible();
    if (Flow.HasOverlay() ||
        (!bFieldOwnedSurface &&
         Flow.Current() != EEchoesShellScreen::Gameplay &&
         Flow.Current() != EEchoesShellScreen::ReplayTransport))
    {
        return true;
    }

    if (Controller.IsCampaignOperationsMapVisible())
    {
        OutView.Surface = EEchoesFieldHudSurface::CampaignOperations;
        OutView.Campaign.bVisible = true;
        OutView.Campaign.Title = LOCTEXT("CampaignMapTitle", "ECHOES OF THE BROKEN SUN // SORYN STRATEGIC OPERATIONS MAP");
        const FEchoesCampaignProgress& Progress = Bridge.GetCampaignProgress();
        OutView.Campaign.Layout = FEchoesCampaignMapLayout::Build(
            Context.ViewportSize, OutView.HudScale, Progress,
            Controller.GetSelectedCampaignMapNodeIndex());
        int32 Harvest = 0;
        int32 Preserve = 0;
        int32 Reshape = 0;
        for (const FEchoesCampaignDecisionRecord& Record : Progress.Decisions)
        {
            Harvest += Record.WellChoice == FutureWellChoice::Harvest ? 1 : 0;
            Preserve += Record.WellChoice == FutureWellChoice::Preserve ? 1 : 0;
            Reshape += Record.WellChoice == FutureWellChoice::Reshape ? 1 : 0;
        }
        OutView.Campaign.LedgerSummary = FText::Format(
            LOCTEXT("CampaignLedger", "SECTORS SECURED: {0}/15 // PROTOCOLS: H:{1} P:{2} R:{3}"),
            FText::AsNumber(OutView.Campaign.Layout.CompletedMissionCount),
            FText::AsNumber(Harvest), FText::AsNumber(Preserve),
            FText::AsNumber(Reshape));
        const int32 Selected = Controller.GetSelectedCampaignMapNodeIndex();
        for (const FEchoesCampaignMapNode& Node : OutView.Campaign.Layout.Nodes)
        {
            FEchoesFieldHudControl NodeControl;
            NodeControl.Label = Text(Node.MissionCode);
            NodeControl.Detail = Text(Node.Title);
            NodeControl.Action = EEchoesFieldHudAction::CampaignSelectNode;
            NodeControl.Argument = Node.Index;
            NodeControl.bFocused = Node.Index == Selected;
            OutView.Campaign.Controls.Add(MoveTemp(NodeControl));
        }
        if (OutView.Campaign.Layout.Nodes.IsValidIndex(Selected))
        {
            const FEchoesCampaignMapNode& Node = OutView.Campaign.Layout.Nodes[Selected];
            OutView.Campaign.SelectedSector = FText::Format(
                LOCTEXT("CampaignSector", "SECTOR {0} // ACT {1}"),
                Text(Node.MissionCode), FText::AsNumber(Node.Act));
            OutView.Campaign.SelectedTitle = Text(Node.Title);
            OutView.Campaign.SelectedBiome = FText::Format(
                LOCTEXT("CampaignBiome", "BIOME: {0}"), Text(Node.BiomeName));
            OutView.Campaign.SelectedStatus = Node.State == EEchoesCampaignNodeState::Completed
                ? LOCTEXT("CampaignSecured", "SECURED // REDEPLOY AVAILABLE")
                : Node.State == EEchoesCampaignNodeState::Available
                    ? LOCTEXT("CampaignReady", "READY FOR DEPLOYMENT")
                    : LOCTEXT("CampaignLocked", "LOCKED // SECURE EARLIER SECTORS");
            FString Briefing = Context.Narrative != nullptr
                ? Context.Narrative->GetBriefing(Node.Operation) : FString();
            if (Briefing.IsEmpty())
            {
                Briefing = LOCTEXT("CampaignBriefingFallback", "Authoritative briefing data is linked in the narrative pack.").ToString();
            }
            OutView.Campaign.Briefing = Text(Briefing);
            if (const FEchoesMissionReward* Reward = FEchoesCampaignRewards::GetReward(Node.MissionId))
            {
                OutView.Campaign.Reward = FText::Format(
                    LOCTEXT("CampaignRewards", "SKIRMISH: {0} // DOCTRINE: {1} // CODEX: {2}"),
                    Text(Reward->SkirmishMapUnlock), Text(Reward->DoctrineUnlock),
                    Text(Reward->CodexUnlock));
            }
            FEchoesFieldHudControl Select;
            Select.Label = Node.State == EEchoesCampaignNodeState::Completed
                ? LOCTEXT("CampaignRedeploy", "REDEPLOY SECTOR")
                : LOCTEXT("CampaignDeploy", "DEPLOY OPERATION");
            Select.Action = EEchoesFieldHudAction::CampaignDeploy;
            Select.bEnabled = Node.State != EEchoesCampaignNodeState::Locked;
            Select.bPrimary = Node.State == EEchoesCampaignNodeState::Available;
            OutView.Campaign.Controls.Add(MoveTemp(Select));
        }
        FEchoesFieldHudControl Back;
        Back.Label = LOCTEXT("CampaignReturn", "RETURN TO TITLE");
        Back.Action = EEchoesFieldHudAction::CampaignBack;
        OutView.Campaign.Controls.Add(MoveTemp(Back));
        ApplySettings(Context.Settings, OutView);
        return true;
    }

    const UEchoesGameInstance* GameInstance =
        Controller.GetWorld() != nullptr
            ? Cast<UEchoesGameInstance>(Controller.GetWorld()->GetGameInstance())
            : nullptr;
    if (Controller.IsOnlineFrontDoorVisible())
    {
        OutView.Surface = EEchoesFieldHudSurface::OnlineFrontDoor;
        OutView.Online.bVisible = true;
        OutView.Online.Title = LOCTEXT("OnlineTitle", "ONLINE // FIXED-RULES DIRECT 1V1");
        EEchoesOnlineFrontDoorState OnlineState =
            EEchoesOnlineFrontDoorState::Idle;
        if (GameInstance != nullptr)
        {
            OutView.Online.Endpoint = Text(GameInstance->GetDirectConnectEndpoint());
            OutView.Online.Failure = Text(GameInstance->GetOnlineFailureMessage());
            OnlineState = GameInstance->GetOnlineState();
            OutView.Online.State = OnlineState == EEchoesOnlineFrontDoorState::Hosting
                ? LOCTEXT("OnlineHosting", "HOSTING")
                : OnlineState == EEchoesOnlineFrontDoorState::Connecting
                    ? LOCTEXT("OnlineConnecting", "CONNECTING")
                    : OnlineState == EEchoesOnlineFrontDoorState::ClientLobby
                        ? LOCTEXT("OnlineClientLobby", "CLIENT LOBBY")
                        : OnlineState == EEchoesOnlineFrontDoorState::Failed
                            ? LOCTEXT("OnlineFailed", "FAILED")
                            : OnlineState == EEchoesOnlineFrontDoorState::JoinSetup
                                ? LOCTEXT("OnlineJoinSetup", "JOIN SETUP")
                                : LOCTEXT("OnlineReadyState", "READY");
            if (OnlineState == EEchoesOnlineFrontDoorState::Hosting &&
                !GameInstance->GetHostShareEndpoint().IsEmpty())
            {
                OutView.Online.Endpoint = Text(GameInstance->GetHostShareEndpoint());
                FEchoesFieldHudControl Copy;
                Copy.Label = LOCTEXT("OnlineCopyHost", "COPY HOST ADDRESS");
                Copy.Action = EEchoesFieldHudAction::OnlineCopyHostAddress;
                OutView.Online.Controls.Add(MoveTemp(Copy));
            }
        }
        if (OnlineState == EEchoesOnlineFrontDoorState::JoinSetup ||
            OnlineState == EEchoesOnlineFrontDoorState::Idle)
        {
            const EEchoesFieldHudAction Actions[] = {
                EEchoesFieldHudAction::OnlineHost,
                EEchoesFieldHudAction::OnlineEditEndpoint,
                EEchoesFieldHudAction::OnlineJoin,
                EEchoesFieldHudAction::OnlineBack};
            const FText Labels[] = {
                LOCTEXT("OnlineHost", "HOST MATCH"),
                LOCTEXT("OnlineEdit", "EDIT ENDPOINT"),
                LOCTEXT("OnlineJoin", "JOIN MATCH"),
                LOCTEXT("OnlineBack", "BACK")};
            const int32 Focus = GameInstance != nullptr ? GameInstance->GetOnlineFocusIndex() : -1;
            for (int32 Index = 0; Index < 4; ++Index)
            {
                FEchoesFieldHudControl Control;
                Control.Label = Labels[Index];
                Control.Action = Actions[Index];
                Control.bFocused = Index == Focus;
                Control.bPrimary = Index == 2;
                OutView.Online.Controls.Add(MoveTemp(Control));
            }
        }
        else if (OnlineState == EEchoesOnlineFrontDoorState::Failed)
        {
            FEchoesFieldHudControl Retry;
            Retry.Label = GameInstance != nullptr && GameInstance->HasUsableReconnectContext()
                ? LOCTEXT("OnlineRejoin", "REJOIN MATCH")
                : LOCTEXT("OnlineRetry", "RETRY ONLINE MENU");
            Retry.Action = EEchoesFieldHudAction::OnlineRetry;
            Retry.bFocused = true;
            Retry.bPrimary = true;
            OutView.Online.Controls.Add(MoveTemp(Retry));
            FEchoesFieldHudControl Back;
            Back.Label = LOCTEXT("OnlineBackOperations", "BACK TO OPERATIONS");
            Back.Action = EEchoesFieldHudAction::OnlineBack;
            OutView.Online.Controls.Add(MoveTemp(Back));
        }
        else
        {
            FEchoesFieldHudControl Back;
            Back.Label = LOCTEXT("OnlineCancel", "CANCEL TO ONLINE MENU");
            Back.Action = EEchoesFieldHudAction::OnlineBack;
            OutView.Online.Controls.Add(MoveTemp(Back));
        }
        ApplySettings(Context.Settings, OutView);
        return true;
    }

    if (Controller.IsNetworkCompatibilityAccepted() && !Controller.IsNetworkMatchStarted())
    {
        OutView.Surface = EEchoesFieldHudSurface::NetworkLobby;
        OutView.Online.bVisible = true;
        OutView.Online.Title = LOCTEXT("OnlineLobbyTitle", "ONLINE LOBBY // GLASS SCAR");
        OutView.Online.State = FText::Format(
            LOCTEXT("OnlineSeat", "CONNECTION-BOUND SEAT {0} // COMPATIBILITY ACCEPTED"),
            FText::AsNumber(Controller.GetNetworkSeat()));
        FEchoesFieldHudControl Ready;
        Ready.Label = LOCTEXT("OnlineReady", "READY AND START MATCH");
        Ready.Action = EEchoesFieldHudAction::NetworkReady;
        Ready.bPrimary = true;
        OutView.Online.Controls.Add(MoveTemp(Ready));
        ApplySettings(Context.Settings, OutView);
        return true;
    }

    if (Controller.IsOnlineLocalMenuVisible())
    {
        OutView.Surface = EEchoesFieldHudSurface::OnlineLocalMenu;
        OutView.Online.bVisible = true;
        OutView.Online.Title = LOCTEXT("OnlineFieldMenu", "ONLINE FIELD MENU");
        OutView.Online.State = LOCTEXT("OnlineAuthorityContinues", "THE AUTHORITY CONTINUES WHILE THIS MENU IS OPEN");
        FEchoesFieldHudControl Resume;
        Resume.Label = LOCTEXT("OnlineResume", "RESUME MATCH");
        Resume.Action = EEchoesFieldHudAction::OnlineResume;
        Resume.bPrimary = true;
        OutView.Online.Controls.Add(MoveTemp(Resume));
        FEchoesFieldHudControl Leave;
        Leave.Label = LOCTEXT("OnlineLeave", "LEAVE ONLINE MATCH");
        Leave.Action = EEchoesFieldHudAction::OnlineLeave;
        OutView.Online.Controls.Add(MoveTemp(Leave));
        ApplySettings(Context.Settings, OutView);
        return true;
    }

    const bool bReconnect = Controller.IsOpponentReconnectGraceActive();
    const TArray<uint32>& Selected = Controller.GetSelectedEntityIds();
    if (Bridge.IsReplayPlaybackActive())
    {
        const FEchoesReplayPlaybackState Playback = Bridge.GetReplayPlaybackState();
        if (Playback.Perspective == EEchoesReplayPerspective::OmniscientObserver)
        {
            if (const Simulation* Replay = Bridge.GetReplayPresentationSimulation())
            {
                OutView = BuildReplayObserver(*Replay);
            }
            else
            {
                OutError = TEXT("[FIELD_HUD_REPLAY_SOURCE_MISSING] Observer replay has no detached presentation state.");
                return false;
            }
        }
        else
        {
            const std::optional<PlayerView> ReplayPlayer =
                Bridge.GetReplayPresentationPlayerView();
            if (!ReplayPlayer.has_value())
            {
                OutError = TEXT("[FIELD_HUD_REPLAY_SCOPE_MISSING] Player replay perspective has no scoped detached view.");
                return false;
            }
            OutView = BuildPlayerScoped(*ReplayPlayer, {}, true);
        }
        ApplySettings(Context.Settings, OutView);
        AddSpatialPresentation(Context, OutView);
        return true;
    }

    if (Controller.IsActiveOnlineNetworkMatch())
    {
        const echoes::sim::net::ScopedViewKeyframe* Keyframe = Controller.GetNetworkScopedView();
        if (Keyframe == nullptr)
        {
            OutError = TEXT("[FIELD_HUD_NETWORK_SOURCE_MISSING] No validated scoped keyframe is available.");
            return false;
        }
        OutView = BuildNetworkScoped(*Keyframe, Selected);
    }
    else
    {
        const Simulation* SimulationValue = Bridge.GetSimulation();
        const std::optional<PlayerView> Player = SimulationValue != nullptr
            ? SimulationValue->CreatePlayerView(UEchoesSimulationSubsystem::LocalPlayerId)
            : std::optional<PlayerView>{};
        if (!Player.has_value())
        {
            OutError = TEXT("[FIELD_HUD_PLAYER_VIEW_MISSING] No live scoped player view is available.");
            return false;
        }
        OutView = BuildPlayerScoped(*Player, Selected, false);
        OutView.Resources.OpponentFaction = Text(Controller.GetOpponentFactionLabel());
        OutView.Resources.MatchState = OutcomeText(SimulationValue->Outcome());
    }
    ApplySettings(Context.Settings, OutView);
    OutView.Status = Text(Controller.GetStatusMessage());
    OutView.Commands.Formation = Text(Controller.GetFormationLabel());
    OutView.Commands.ArmedAction = Controller.GetArmedDeckAction();
    if (OutView.Authority == EEchoesFieldHudAuthority::LivePlayerView)
    {
        BuildCommandControls(
            Controller.BuildCommandDeckProfile(), OutView.Commands);
    }
    if (const auto Subgroup = Controller.GetActiveSelectionSubgroupType(); Subgroup.IsSet())
        OutView.Commands.Formation = FText::Format(LOCTEXT("ActiveSubgroup", "Active subgroup: {0}"), EntityName(Subgroup.GetValue()));
    OutView.Targeting.bKeyboardTargetVisible = Controller.IsKeyboardTargetingEnabled();
    OutView.Targeting.KeyboardTargetNormalizedOffset = FVector2D(
        Controller.GetKeyboardTargetOffset().X /
            FMath::Max(1.0f, Context.ViewportSize.X),
        Controller.GetKeyboardTargetOffset().Y /
            FMath::Max(1.0f, Context.ViewportSize.Y));
    OutView.Targeting.bSelectionDragVisible = Controller.IsDraggingSelection();
    OutView.Targeting.SelectionStartNormalized = FVector2D(
        Controller.GetSelectionStartScreenPosition().X /
            FMath::Max(1.0f, Context.ViewportSize.X),
        Controller.GetSelectionStartScreenPosition().Y /
            FMath::Max(1.0f, Context.ViewportSize.Y));
    OutView.Targeting.SelectionEndNormalized = FVector2D(
        Controller.GetSelectionCurrentScreenPosition().X /
            FMath::Max(1.0f, Context.ViewportSize.X),
        Controller.GetSelectionCurrentScreenPosition().Y /
            FMath::Max(1.0f, Context.ViewportSize.Y));
    if (OutView.Authority == EEchoesFieldHudAuthority::LivePlayerView)
    {
        const Simulation* SimulationValue = Bridge.GetSimulation();
        const std::optional<PlayerView> Player = SimulationValue != nullptr
            ? SimulationValue->CreatePlayerView(UEchoesSimulationSubsystem::LocalPlayerId)
            : std::optional<PlayerView>{};
        if (Player.has_value())
        {
            BuildTechnology(*Player, Selected,
                Controller.GetTechnologyPanelFocusedTier(),
                Controller.IsTechnologyPanelVisible(), OutView.Technology);
            if (OutView.Technology.bVisible)
            {
                OutView.Selection = FEchoesFieldHudSelectionView{};
                OutView.Production = FEchoesFieldHudProductionView{};
                OutView.Commands = FEchoesFieldHudCommandView{};
                OutView.Targeting = FEchoesFieldHudTargetingView{};
            }
        }
        FEchoesFieldHudProductionCancellationView Cancellation;
        if (Controller.GetProductionCancellationConfirmation(Cancellation) &&
            OutView.Production.bVisible &&
            OutView.Production.ProducerId == Cancellation.ProducerId)
        {
            OutView.Production.Cancellation = Cancellation;
            OutView.Production.Items.Reset();
            OutView.Production.Controls.Reset();

            FEchoesFieldHudControl Back;
            Back.Label = LOCTEXT("ProductionCancellationBack", "BACK");
            Back.Detail = LOCTEXT(
                "ProductionCancellationBackDetail",
                "Keep this production order");
            Back.Action = EEchoesFieldHudAction::ProductionCancelBack;
            Back.bPrimary = true;
            OutView.Production.Controls.Add(MoveTemp(Back));

            FEchoesFieldHudControl Confirm;
            Confirm.Label = LOCTEXT(
                "ProductionCancellationConfirm", "CONFIRM CANCELLATION");
            Confirm.Detail = FText::Format(
                LOCTEXT(
                    "ProductionCancellationConfirmDetail",
                    "Refund {0} Matter / {1} Dawn"),
                FText::AsNumber(Cancellation.RefundMatter),
                FText::AsNumber(Cancellation.RefundDawn));
            Confirm.Action = EEchoesFieldHudAction::ProductionCancelConfirm;
            OutView.Production.Controls.Add(MoveTemp(Confirm));

            OutView.Commands = FEchoesFieldHudCommandView{};
            OutView.Targeting = FEchoesFieldHudTargetingView{};
        }
        const FEchoesObjectiveSnapshot Objective = Bridge.GetLocalObjectiveSnapshot();
        BuildObjectiveView(Objective, OutView);
        BuildMissionMarkers(Bridge, Objective, OutView.Minimap);
        if (Bridge.GetOperationMode() == EEchoesOperationMode::TrainingReadiness)
        {
            const uint16 PracticeTarget = Controller.GetTutorialPracticeTargetBit();
            const uint16 Mask = PracticeTarget != 0
                ? static_cast<uint16>(
                    FEchoesTutorialPracticeState::ImplementedLessonMask &
                    ~PracticeTarget)
                : Controller.GetTutorialProgressMask();
            if ((Mask & 7) == 7 && (Mask & 8) == 0)
            {
                OutView.Minimap.MissionMarkers.Add({Normalize(Vec2::FromTiles(14, 18), OutView.Minimap.Width, OutView.Minimap.Height), LOCTEXT("RouteMarker", "R"), EEchoesFieldHudTone::Accent});
                OutView.Minimap.MissionMarkers.Add({Normalize(Vec2::FromTiles(14, 20), OutView.Minimap.Width, OutView.Minimap.Height), LOCTEXT("PatrolMarker", "P"), EEchoesFieldHudTone::Accent});
                OutView.Minimap.MissionMarkers.Add({Normalize(Vec2::FromTiles(19, 10), OutView.Minimap.Width, OutView.Minimap.Height), LOCTEXT("RejectionMarker", "X"), EEchoesFieldHudTone::Warning});
                if (Controller.GetTutorialPendingRejectionAttempt() != 0)
                    OutView.ObjectiveControls.Add({LOCTEXT("ReadRejection", "Acknowledge blocked route"), FText::GetEmpty(), EEchoesFieldHudAction::AcknowledgeTutorialRejection});
            }
            if ((Mask & 15) == 15 && (Mask & 16) == 0)
                OutView.ObjectiveControls.Add({LOCTEXT("InspectReserve", "Inspect reserve monitor"), FText::GetEmpty(), EEchoesFieldHudAction::InspectTutorialReserve});
        }
        const FText Tutorial = Controller.GetTutorialInstruction();
        OutView.TutorialInstruction = Tutorial;
        if (!Tutorial.IsEmpty())
        {
            OutView.ObjectiveTitle = LOCTEXT("TutorialReadinessTitle", "READINESS CHECK");
            OutView.ObjectiveLines.Insert({FText::GetEmpty(), Tutorial, EEchoesFieldHudTone::Accent}, 0);
            OutView.bObjectiveVisible = true;
        }
        if (OutView.Production.Cancellation.bVisible)
        {
            OutView.ObjectiveControls.Reset();
        }
        OutView.bTutorialActive = Controller.IsTutorialOperationAuthorized() && !Controller.IsReplayInputActive();
        if (Controller.IsTutorialSkipModalVisible())
        {
            OutView.TutorialSkipModal.bVisible = true;
            OutView.TutorialSkipModal.Title = LOCTEXT("TutorialSkipModalTitle", "TUTORIAL OPTIONS");
            OutView.TutorialSkipModal.Description = LOCTEXT("TutorialSkipModalDesc", "Choose an option to manage the tutorial guidance:");
            OutView.TutorialSkipModal.Controls.Add({LOCTEXT("SkipStepBtn", "Skip this step only"), FText::GetEmpty(), EEchoesFieldHudAction::TutorialSkipCurrentStep, 0, true, false, false});
            OutView.TutorialSkipModal.Controls.Add({LOCTEXT("EndAllBtn", "End all tutorials"), FText::GetEmpty(), EEchoesFieldHudAction::TutorialEndAll, 0, true, false, false});
            OutView.TutorialSkipModal.Controls.Add({LOCTEXT("CancelSkipBtn", "Cancel"), FText::GetEmpty(), EEchoesFieldHudAction::TutorialCancelSkipModal, 0, true, true, true});
        }
        else if (OutView.bTutorialActive && Bridge.GetOperationMode() == EEchoesOperationMode::TrainingReadiness)
        {
            const uint16 Mask = Controller.GetTutorialProgressMask();
            if ((Mask & 1) == 0)
            {
                OutView.TutorialLessonTitle = LOCTEXT("Lesson1Title", "LESSON 1: SURVEY");
                const auto& Survey = Controller.GetTutorialSurvey();
                const int32 Waypoint = Survey.CompletedWaypoints();
                FVector AnchorWorld = Bridge.SimToWorld(UEchoesSimulationSubsystem::GetArchiveRecoverySite());
                if (const auto* Simulation = Bridge.GetSimulation())
                {
                    if (const auto View = Simulation->CreatePlayerView(UEchoesSimulationSubsystem::LocalPlayerId))
                    {
                        for (const auto& Entity : View->Entities())
                        {
                            if (Entity.owner == UEchoesSimulationSubsystem::LocalPlayerId &&
                                Entity.type == echoes::sim::EntityType::CommandCore)
                            {
                                AnchorWorld = Bridge.SimToWorld(Entity.position);
                                break;
                            }
                        }
                    }
                }

                FVector TargetWorld = AnchorWorld;
                FText TargetName = LOCTEXT("SpotlightAnchor", "Anchor");
                FText ActionPrompt = BoundTutorialText(LOCTEXT("ActionSelectAnchor", "Use {select_key} to select Anchor"));
                FText InputBinding = BoundTutorialText(LOCTEXT("BindingSelectAnchor", "{select_key}"));

                if (!Controller.IsTutorialCoreSelected())
                {
                    TargetWorld = AnchorWorld;
                    TargetName = LOCTEXT("SpotlightAnchor", "Anchor");
                    ActionPrompt = BoundTutorialText(LOCTEXT("ActionSelectAnchor", "Use {select_key} to select Anchor"));
                    InputBinding = BoundTutorialText(LOCTEXT("BindingSelectAnchor", "{select_key}"));
                }
                else if (Waypoint == 0)
                {
                    TargetWorld = AnchorWorld;
                    TargetName = LOCTEXT("SpotlightAnchor", "Anchor");
                    if (!Survey.HasPanned())
                    {
                        ActionPrompt = BoundTutorialText(LOCTEXT("ActionPanCamera", "Pan camera with {pan_keys}"));
                        InputBinding = BoundTutorialText(LOCTEXT("BindingPan", "{pan_keys}"));
                    }
                    else if (!(Survey.HasZoomedMin() && Survey.HasZoomedMax()))
                    {
                        ActionPrompt = BoundTutorialText(LOCTEXT("ActionZoomCamera", "Zoom with {zoom_in_key} / {zoom_out_key}"));
                        InputBinding = BoundTutorialText(LOCTEXT("BindingZoom", "{zoom_in_key} / {zoom_out_key}"));
                    }
                    else
                    {
                        ActionPrompt = LOCTEXT("ActionRecenterAnchor", "Center Camera on Anchor (1.5s)");
                        InputBinding = BoundTutorialText(LOCTEXT("BindingRecenter", "{recenter_key}"));
                    }
                }
                else if (Waypoint == 1)
                {
                    TargetWorld = Bridge.SimToWorld(UEchoesSimulationSubsystem::GetArchiveRecoverySite());
                    TargetName = LOCTEXT("SpotlightArchive", "Archive Recovery Site");
                    ActionPrompt = LOCTEXT("ActionCenterArchive", "Center Camera Here (1.5s)");
                    InputBinding = BoundTutorialText(LOCTEXT("BindingCenterArchive", "{pan_keys} / Minimap"));
                }
                else if (Waypoint == 2)
                {
                    TargetWorld = Bridge.SimToWorld(UEchoesSimulationSubsystem::GetEvacuationSite());
                    TargetName = LOCTEXT("SpotlightEvac", "Evacuation Site");
                    ActionPrompt = LOCTEXT("ActionCenterEvac", "Center Camera Here (1.5s)");
                    InputBinding = BoundTutorialText(LOCTEXT("BindingCenterEvac", "{pan_keys} / Minimap"));
                }
                else
                {
                    TargetWorld = AnchorWorld;
                    TargetName = LOCTEXT("SpotlightAnchor", "Anchor");
                    ActionPrompt = BoundTutorialText(LOCTEXT("ActionCompleteSurvey", "Use {select_key} on Anchor to complete"));
                    InputBinding = BoundTutorialText(LOCTEXT("BindingSelectAnchor", "{select_key}"));
                }

                FVector2D ScreenPos;
                if (Controller.ProjectWorldLocationToScreen(TargetWorld, ScreenPos))
                {
                    OutView.TutorialSpotlight.bActive = true;
                    OutView.TutorialSpotlight.ScreenCenter = ScreenPos;
                    OutView.TutorialSpotlight.ScreenSize = FVector2D(140.0f, 140.0f);
                    OutView.TutorialSpotlight.TargetName = TargetName;
                    OutView.TutorialSpotlight.ActionPrompt = ActionPrompt;
                    OutView.TutorialSpotlight.InputBinding = InputBinding;
                }
            }
            else if ((Mask & 2) == 0)
            {
                OutView.TutorialLessonTitle = LOCTEXT("Lesson2Title", "LESSON 2: ROSTER");
                if (Controller.GetSelectedEntityIds().IsEmpty())
                {
                    if (const auto* Simulation = Bridge.GetSimulation())
                    {
                        if (const auto View = Simulation->CreatePlayerView(UEchoesSimulationSubsystem::LocalPlayerId))
                        {
                            for (const auto& Entity : View->Entities())
                            {
                                if (Entity.owner == UEchoesSimulationSubsystem::LocalPlayerId &&
                                    Entity.type == echoes::sim::EntityType::Worker)
                                {
                                    FVector2D ScreenPos;
                                    if (Controller.ProjectWorldLocationToScreen(Bridge.SimToWorld(Entity.position), ScreenPos))
                                    {
                                        OutView.TutorialSpotlight.bActive = true;
                                        OutView.TutorialSpotlight.ScreenCenter = ScreenPos;
                                        OutView.TutorialSpotlight.ScreenSize = FVector2D(120.0f, 120.0f);
                                        OutView.TutorialSpotlight.TargetName = LOCTEXT("SpotlightSurveyor", "Surveyor");
                                        OutView.TutorialSpotlight.ActionPrompt = BoundTutorialText(LOCTEXT("ActionSelectSurveyor", "Use {select_key} to select Surveyor"));
                                        OutView.TutorialSpotlight.InputBinding = BoundTutorialText(LOCTEXT("BindingLeftClick", "{select_key}"));
                                    }
                                    break;
                                }
                            }
                        }
                    }
                }
                else
                {
                    const auto Progress = Controller.GetTutorialSelection().RosterProgress();
                    if (!Progress.bHudPublished)
                    {
                        OutView.TutorialSpotlight.bActive = true;
                        OutView.TutorialSpotlight.ScreenCenter = FVector2D(530.0f, 610.0f);
                        OutView.TutorialSpotlight.ScreenSize = FVector2D(520.0f, 180.0f);
                        OutView.TutorialSpotlight.TargetName = LOCTEXT("SpotlightSelectionPanel", "Unit Card & Commands");
                        OutView.TutorialSpotlight.ActionPrompt = LOCTEXT("ActionReadHud", "Reviewing Unit Stats and Orders");
                        OutView.TutorialSpotlight.InputBinding = LOCTEXT("BindingInspect", "Keep Selected");
                    }
                    else
                    {
                        OutView.TutorialSpotlight.bActive = true;
                        OutView.TutorialSpotlight.ScreenCenter = FVector2D(400.0f, 300.0f);
                        OutView.TutorialSpotlight.ScreenSize = FVector2D(100.0f, 100.0f);
                        OutView.TutorialSpotlight.TargetName = LOCTEXT("SpotlightGround", "Clear Ground");
                        OutView.TutorialSpotlight.ActionPrompt = BoundTutorialText(LOCTEXT("ActionDeselect", "Use {select_key} on ground to deselect"));
                        OutView.TutorialSpotlight.InputBinding = BoundTutorialText(LOCTEXT("BindingLeftClick", "{select_key}"));
                    }
                }
            }
            else if ((Mask & 4) == 0)
            {
                OutView.TutorialLessonTitle = LOCTEXT("Lesson3Title", "LESSON 3: MUSTER");
                const auto Progress = Controller.GetTutorialSelection().MusterProgress();
                OutView.TutorialSpotlight.bActive = true;
                OutView.TutorialSpotlight.ScreenCenter = FVector2D(640.0f, 360.0f);
                OutView.TutorialSpotlight.ScreenSize = FVector2D(360.0f, 260.0f);
                OutView.TutorialSpotlight.TargetName = LOCTEXT("SpotlightMusterGroup", "Surveyors & Lancers");
                if (!Progress.bDragSelected)
                {
                    OutView.TutorialSpotlight.ActionPrompt = LOCTEXT("ActionDragBox", "Click & Drag Box Around All 8 Units");
                    OutView.TutorialSpotlight.InputBinding = BoundTutorialText(LOCTEXT("BindingDrag", "Hold {select_key} and drag"));
                }
                else if (!Progress.bControlGroupAssigned)
                {
                    OutView.TutorialSpotlight.ActionPrompt = BoundTutorialText(LOCTEXT("ActionAssignGroup", "Press {assign_group_key}, then {recall_group_key}"));
                    OutView.TutorialSpotlight.InputBinding = BoundTutorialText(LOCTEXT("BindingGroup", "{assign_group_key}, then {recall_group_key}"));
                }
                else
                {
                    OutView.TutorialSpotlight.ActionPrompt = BoundTutorialText(LOCTEXT("ActionRecallGroup", "Use {recall_group_key} to recall group"));
                    OutView.TutorialSpotlight.InputBinding = BoundTutorialText(LOCTEXT("BindingRecall", "{recall_group_key}"));
                }
            }
            else if ((Mask & 8) == 0)
            {
                OutView.TutorialLessonTitle = LOCTEXT("Lesson4Title", "LESSON 4: ROUTE");
            }
            else if ((Mask & 16) == 0)
            {
                OutView.TutorialLessonTitle = LOCTEXT("Lesson5Title", "LESSON 5: RESERVE");
            }
        }
    }
    if (Context.Narrative != nullptr)
    {
        FString Speaker;
        FString Subtitle;
        if (Context.Narrative->GetActiveSubtitle(Context.RealTimeSeconds, Speaker, Subtitle))
        {
            OutView.SubtitleSpeaker = Text(Speaker);
            OutView.Subtitle = Text(Subtitle);
        }
    }
    if (bReconnect)
    {
        OutView.Surface = EEchoesFieldHudSurface::Reconnect;
        OutView.Online.bVisible = true;
        OutView.Online.Title = LOCTEXT("ReconnectPaused", "OPPONENT DISCONNECTED // AUTHORITY PAUSED");
        const int32 Remaining = Controller.GetOpponentReconnectSecondsRemaining();
        OutView.Online.Reconnect = FText::Format(
            LOCTEXT("ReconnectTime", "RECONNECT {0}:{1}"),
            FText::AsNumber(Remaining / 60),
            Text(FString::Printf(TEXT("%02d"), Remaining % 60)));
    }
    AddSpatialPresentation(Context, OutView);
    const auto* Cinematic = Controller.GetWorld()
        ? Controller.GetWorld()->GetSubsystem<UEchoesCinematicSubsystem>() : nullptr;
    if (Cinematic && Cinematic->IsSequenceActive(EEchoesCinematicSequence::M01Opening))
    {
        // The opening retains subtitles and its skip/pause instruction. The
        // live command board returns when Sequencer releases control.
        OutView.Resources = {};
        OutView.Selection = {};
        OutView.Commands = {};
        OutView.Technology = {};
        OutView.Minimap = {};
        OutView.Targeting = {};
        OutView.Status = FText::GetEmpty();
        OutView.ObjectiveLines.Reset();
        OutView.ObjectiveControls.Reset();
        OutView.ObjectiveTitle = LOCTEXT("OpeningTitle", "WHAT THE LEDGER KEEPS");
        const FText Instruction = Controller.GetTutorialInstruction();
        OutView.bObjectiveVisible = !Instruction.IsEmpty();
        if (!Instruction.IsEmpty())
            OutView.ObjectiveLines.Add({FText::GetEmpty(), Instruction, EEchoesFieldHudTone::Muted});
    }
    return true;
}

#undef LOCTEXT_NAMESPACE

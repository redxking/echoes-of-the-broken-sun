#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "EchoesHudLayout.h"

#include "EchoesFieldHudWidget.h"
#include "Components/ProgressBar.h"
#include "EchoesTestSaveEnvironment.h"

#include "Blueprint/UserWidget.h"
#include "Components/TextBlock.h"
#include "Components/CanvasPanelSlot.h"
#include "Input/HittestGrid.h"
#include "Misc/App.h"
#include "Rendering/DrawElements.h"
#include "Tests/AutomationCommon.h"
#include "Types/PaintArgs.h"
#include "Widgets/SWindow.h"

namespace
{
int32 CountFieldDrawElements(const FSlateDrawElementMap& ElementMap)
{
    int32 Total = 0;
    VisitTupleElements(
        [&Total](const auto& Elements)
        {
            Total += Elements.Num();
        },
        ElementMap);
    return Total;
}

FEchoesFieldHudControl FieldControl(
    const TCHAR* Label,
    EEchoesFieldHudAction Action,
    int32 Argument,
    bool bEnabled = true)
{
    FEchoesFieldHudControl Control;
    Control.Label = FText::FromString(Label);
    Control.Action = Action;
    Control.Argument = Argument;
    Control.bEnabled = bEnabled;
    return Control;
}

FEchoesFieldHudView BattlefieldView(float Scale)
{
    FEchoesFieldHudView View;
    View.Authority = EEchoesFieldHudAuthority::LivePlayerView;
    View.Surface = EEchoesFieldHudSurface::Battlefield;
    View.HudScale = Scale;
    View.Resources.bVisible = true;
    View.Resources.Matter = 420;
    View.Resources.Dawn = 31;
    View.Resources.PopulationUsed = 18;
    View.Resources.PopulationCapacity = 40;
    View.Resources.SimulationTick = 77;
    View.Resources.LocalFaction = FText::FromString(TEXT("MERIDIAN"));
    View.Resources.OpponentFaction = FText::FromString(TEXT("KHARUUN"));
    View.Resources.MatchState = FText::FromString(TEXT("NETWORK ACTIVE"));
    View.Resources.ResearchStatus = FText::FromString(TEXT("RESEARCH: FIELD LATTICE"));
    View.Resources.MonitorControl = FieldControl(
        TEXT("OPEN RESOURCE MONITOR"),
        EEchoesFieldHudAction::OpenResourceMonitor, 0);
    View.Resources.MonitorControl.Detail = FText::FromString(
        TEXT("Review economy and commitments"));

    View.Selection.bVisible = true;
    FEchoesFieldHudSelectionEntry Selection;
    Selection.EntityId = 12;
    Selection.Name = FText::FromString(TEXT("Civic Lancer"));
    Selection.Order = FText::FromString(TEXT("HOLDING"));
    Selection.HitPoints = 82;
    Selection.MaxHitPoints = 100;
    Selection.Damage = 12;
    Selection.Armor = 3;
    Selection.bOwned = true;
    View.Selection.Entries.Add(Selection);

    View.Production.bVisible = true;
    View.Production.ProducerId = 12;
    View.Production.bSpawnBlocked = true;
    View.Production.RallyWaypointCount = 2;
    FEchoesFieldHudProductionItem Active;
    Active.Slot = 0;
    Active.ItemId = 9001;
    Active.Unit = FText::FromString(TEXT("Surveyor"));
    Active.ProgressPercent = 45;
    Active.InvestedMatter = 50;
    Active.InvestedDawn = 0;
    Active.Logistics = 1;
    Active.bActive = true;
    View.Production.Items.Add(Active);
    FEchoesFieldHudProductionItem Waiting;
    Waiting.Slot = 1;
    Waiting.ItemId = 9002;
    Waiting.Unit = FText::FromString(TEXT("Civic Lancer"));
    Waiting.ConfiguredMatter = 85;
    Waiting.ConfiguredDawn = 20;
    Waiting.Logistics = 2;
    View.Production.Items.Add(Waiting);
    View.Production.Controls = {
        FieldControl(TEXT("REVIEW ACTIVE CANCELLATION"),
            EEchoesFieldHudAction::ProductionCancel, 0),
        FieldControl(TEXT("REVIEW CANCEL 1"),
            EEchoesFieldHudAction::ProductionCancel, 1),
        FieldControl(TEXT("MOVE 1 UP"),
            EEchoesFieldHudAction::ProductionMoveUp, 1, false),
        FieldControl(TEXT("MOVE 1 DOWN"),
            EEchoesFieldHudAction::ProductionMoveDown, 1, false)};

    View.Menu.bVisible = true;
    View.Menu.Control = FieldControl(TEXT("MENU"),
        EEchoesFieldHudAction::OpenPauseMenu, 0);
    View.Commands.bVisible = true;
    View.Commands.Formation = FText::FromString(TEXT("LINE"));
    for (int32 Index = 0; Index < 9; ++Index)
    {
        View.Commands.Controls.Add(FieldControl(
            *FString::Printf(TEXT("COMMAND %d"), Index + 1),
            EEchoesFieldHudAction::CommandDeck,
            Index,
            Index != 7));
    }
    View.ObjectiveTitle = FText::FromString(TEXT("DIRECTIVES"));
    View.bObjectiveVisible = true;
    View.ObjectiveLines.Add({FText::FromString(TEXT("PRIMARY")),
        FText::FromString(TEXT("Secure the Well")),
        EEchoesFieldHudTone::Accent});
    View.Status = FText::FromString(TEXT("ORDER ACCEPTED"));
    View.SubtitleSpeaker = FText::FromString(TEXT("MARA VEY"));
    View.Subtitle = FText::FromString(TEXT("Hold the reserve margin."));

    View.Minimap.bVisible = true;
    View.Minimap.Width = 4;
    View.Minimap.Height = 4;
    View.Minimap.Tiles = {
        EEchoesFieldHudTileState::VisibleOpen,
        EEchoesFieldHudTileState::VisibleOpen,
        EEchoesFieldHudTileState::VisibleBlocked,
        EEchoesFieldHudTileState::Unexplored,
        EEchoesFieldHudTileState::ExploredOpen,
        EEchoesFieldHudTileState::ExploredBlocked,
        EEchoesFieldHudTileState::VisibleOpen,
        EEchoesFieldHudTileState::Unexplored,
        EEchoesFieldHudTileState::VisibleOpen,
        EEchoesFieldHudTileState::VisibleOpen,
        EEchoesFieldHudTileState::VisibleOpen,
        EEchoesFieldHudTileState::ExploredOpen,
        EEchoesFieldHudTileState::Unexplored,
        EEchoesFieldHudTileState::ExploredOpen,
        EEchoesFieldHudTileState::VisibleBlocked,
        EEchoesFieldHudTileState::VisibleOpen};
    FEchoesFieldHudMapMarker Friendly;
    Friendly.EntityId = 12;
    Friendly.NormalizedPosition = FVector2D(0.25f, 0.75f);
    Friendly.bFriendly = true;
    View.Minimap.Markers.Add(Friendly);
    FEchoesFieldHudMapMarker Hostile;
    Hostile.EntityId = 90;
    Hostile.NormalizedPosition = FVector2D(0.75f, 0.25f);
    View.Minimap.Markers.Add(Hostile);
    FEchoesFieldHudContact Contact;
    Contact.NormalizedMapPosition = FVector2D(0.50f, 0.50f);
    Contact.NormalizedScreenPosition = FVector2D(0.50f, 0.35f);
    Contact.PrimaryLabel = FText::FromString(TEXT("VIBRATION 01"));
    Contact.bScreenPlacementValid = true;
    View.Minimap.Contacts.Add(Contact);
    FEchoesFieldHudMissionMarker MissionMarker;
    MissionMarker.NormalizedMapPosition = FVector2D(0.40f, 0.60f);
    MissionMarker.Label = FText::FromString(TEXT("WELL"));
    View.Minimap.MissionMarkers.Add(MissionMarker);
    View.Minimap.CameraFrustum = {
        FVector2D(0.20f, 0.20f), FVector2D(0.62f, 0.18f),
        FVector2D(0.70f, 0.65f), FVector2D(0.25f, 0.72f)};
    View.Targeting.bSelectionDragVisible = true;
    View.Targeting.SelectionStartNormalized = FVector2D(0.23f, 0.29f);
    View.Targeting.SelectionEndNormalized = FVector2D(0.41f, 0.60f);
    return View;
}
} // namespace

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEchoesFieldHudWidgetTest,
    "Echoes.Runtime.UI.FieldHudWidget",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::ClientContext |
        EAutomationTestFlags::EngineFilter)

bool FEchoesFieldHudWidgetTest::RunTest(const FString& Parameters)
{
    (void)Parameters;
    FEchoesScopedTestSaveEnvironment Storage(*this);
    if (!Storage.IsReady())
    {
        return false;
    }
    FTestWorldWrapper WorldWrapper;
    if (!WorldWrapper.CreateTestWorld(EWorldType::Game))
    {
        WorldWrapper.ForwardErrorMessages(this);
        AddError(TEXT("Could not create the field-HUD test world."));
        return false;
    }
    UEchoesFieldHudWidget* Widget = CreateWidget<UEchoesFieldHudWidget>(
        WorldWrapper.GetTestWorld(),
        UEchoesFieldHudWidget::StaticClass(),
        TEXT("FieldHudUnderTest"));
    if (!TestNotNull(TEXT("Native field HUD is created"), Widget))
    {
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }
    Widget->Configure(nullptr);
    Widget->SetView(BattlefieldView(0.8f));
    const TSharedRef<SWidget> SlateWidget = Widget->TakeWidget();
    UWidget* InitialRoot = Widget->GetRootWidget();
    TestNotNull(TEXT("Field HUD owns a native UMG root"), InitialRoot);
    auto* ResourcePanel = Widget->GetSection(EEchoesFieldHudSection::ResourceLedger);
    TestEqual(TEXT("Critical resources have three independent readouts"), ResourcePanel->GetResourceReadoutCount(), 3);
    TestFalse(TEXT("Critical resource values never require scrolling"), ResourcePanel->UsesScrollableContent());
    if (ResourcePanel->GetResourceReadoutCount() != 3) return false;
    TestEqual(TEXT("Matter displays the actual scoped balance"), ResourcePanel->GetResourceReadout(0)->GetText().ToString(), FString(TEXT("420")));
    TestEqual(TEXT("Dawn displays the actual scoped balance"), ResourcePanel->GetResourceReadout(1)->GetText().ToString(), FString(TEXT("31")));
    TestEqual(TEXT("Logistics keeps used and capacity together"), ResourcePanel->GetResourceReadout(2)->GetText().ToString(), FString(TEXT("18/40")));
    TestEqual(TEXT("Resource identity is visibly retained from the scoped factions"),
        ResourcePanel->GetResourceIdentityReadout()->GetText().ToString(),
        FString(TEXT("MERIDIAN  //  KHARUUN")));
    TestEqual(TEXT("Resource context visibly retains the actual match and research state"),
        ResourcePanel->GetResourceContextReadout()->GetText().ToString(),
        FString(TEXT("NETWORK ACTIVE  //  RESEARCH: FIELD LATTICE")));
    UEchoesFieldHudActionButton* ResourceAction =
        ResourcePanel->GetResourceActionButton();
    TestTrue(TEXT("The whole resource ledger is one focusable semantic monitor action"),
        ResourceAction != nullptr && ResourcePanel->GetActionButtonCount() == 1 &&
            ResourceAction->GetAction() == EEchoesFieldHudAction::OpenResourceMonitor &&
            ResourceAction->TakeWidget()->SupportsKeyboardFocus() &&
            ResourceAction->GetToolTipText().ToString().Contains(TEXT("Open resource monitor")));
    auto* OriginalMatterReadout = ResourcePanel->GetResourceReadout(0);
    auto ResourceRefresh = BattlefieldView(.8f);
    ResourceRefresh.Resources.Matter = 17;
    Widget->SetView(ResourceRefresh);
    TestTrue(TEXT("Balance updates preserve the existing readout widget"), ResourcePanel->GetResourceReadout(0) == OriginalMatterReadout);
    TestEqual(TEXT("Balance update reaches the visible readout"), OriginalMatterReadout->GetText().ToString(), FString(TEXT("17")));
    Widget->SetView(BattlefieldView(.8f));
    TestEqual(TEXT("Every semantic field panel is a modular child widget"),
        Widget->GetSectionCount(), 12);
    for (uint8 Index = 0;
         Index <= static_cast<uint8>(EEchoesFieldHudSection::TutorialModal);
         ++Index)
    {
        TestNotNull(TEXT("Semantic panel role is materialized"),
            Widget->GetSection(static_cast<EEchoesFieldHudSection>(Index)));
    }
    TestNotNull(TEXT("Minimap is a dedicated geometry widget"),
        Widget->GetMinimapWidget());
    TestNotNull(TEXT("Campaign map is a dedicated geometry widget"),
        Widget->GetCampaignMapWidget());
    TestNotNull(TEXT("Targeting is a dedicated geometry widget"),
        Widget->GetTargetingWidget());
    TestNotNull(TEXT("Battlefield Menu is a real focusable action button"),
        Widget->GetMenuButton());
    TestEqual(TEXT("Battlefield Menu preserves its semantic pause action"),
        Widget->GetMenuButton()->GetAction(),
        EEchoesFieldHudAction::OpenPauseMenu);
    TestEqual(TEXT("Battlefield Menu is visible immediately on a live view"),
        Widget->GetMenuButton()->GetVisibility(), ESlateVisibility::Visible);
    FEchoesFieldHudView HiddenMenuView = BattlefieldView(0.8f);
    HiddenMenuView.Surface = EEchoesFieldHudSurface::Hidden;
    Widget->SetView(HiddenMenuView);
    TestEqual(TEXT("Battlefield Menu collapses immediately when the HUD hides"),
        Widget->GetMenuButton()->GetVisibility(), ESlateVisibility::Collapsed);
    Widget->SetView(BattlefieldView(0.8f));
    TestEqual(TEXT("Battlefield Menu restores on the next live view"),
        Widget->GetMenuButton()->GetVisibility(), ESlateVisibility::Visible);
    TestEqual(TEXT("Anonymous contact has a player-scoped UMG label"),
        Widget->GetContactWidgetCount(), 1);
    TestEqual(TEXT("The tactical command card exposes all nine UMG controls"),
        Widget->GetSection(EEchoesFieldHudSection::CommandCard)
            ->GetActionButtonCount(),
        9);
    UEchoesFieldHudSectionWidget* SelectionPanel =
        Widget->GetSection(EEchoesFieldHudSection::Selection);
    TestEqual(TEXT("Selection has one health track per authoritative entry"), SelectionPanel->GetHealthReadoutCount(), 1);
    UProgressBar* HealthTrack = SelectionPanel->GetHealthReadout(0);
    if (!TestNotNull(TEXT("Selection health telemetry exists"), HealthTrack)) return false;
    TestEqual(TEXT("Health fill consumes actual selected health"), HealthTrack->GetPercent(), .82f);
    FEchoesFieldHudView DamagedView = BattlefieldView(.8f);
    DamagedView.Selection.Entries[0].HitPoints = 25;
    Widget->SetView(DamagedView);
    TestTrue(TEXT("Health changes preserve the active telemetry widget"), SelectionPanel->GetHealthReadout(0) == HealthTrack);
    TestEqual(TEXT("Damage updates the health fill without rebuilding controls"), HealthTrack->GetPercent(), .25f);
    Widget->SetView(BattlefieldView(.8f));
    TestEqual(TEXT("Selected producer exposes every typed queue control"),
        SelectionPanel->GetActionButtonCount(), 4);
    UEchoesFieldHudActionButton* CancelWaiting =
        SelectionPanel->GetActionButton(1);
    TestTrue(TEXT("Queue cancellation is a focusable semantic button"),
        CancelWaiting != nullptr && CancelWaiting->GetIsEnabled() &&
        CancelWaiting->TakeWidget()->SupportsKeyboardFocus() &&
        CancelWaiting->GetAction() ==
            EEchoesFieldHudAction::ProductionCancel &&
        CancelWaiting->GetArgument() == 1);
    UEchoesFieldHudActionButton* MoveUp = SelectionPanel->GetActionButton(2);
    UEchoesFieldHudActionButton* MoveDown = SelectionPanel->GetActionButton(3);
    TestTrue(TEXT("One-item reorder directions remain present for keyboard discovery"),
        MoveUp != nullptr && MoveDown != nullptr);
    TestTrue(TEXT("Impossible one-item reorder directions stay disabled"),
        MoveUp != nullptr && MoveDown != nullptr &&
        !MoveUp->GetIsEnabled() && !MoveDown->GetIsEnabled());

    if (MoveUp != nullptr)
        TestFalse(TEXT("Disabled control still refuses activation"), MoveUp->Activate());

    FEchoesFieldHudView Cancellation = BattlefieldView(0.8f);
    Cancellation.Commands = {};
    Cancellation.Production.Items.Reset();
    Cancellation.Production.Cancellation.bVisible = true;
    Cancellation.Production.Cancellation.ProducerId = 12;
    Cancellation.Production.Cancellation.ItemId = 9001;
    Cancellation.Production.Cancellation.Unit =
        FText::FromString(TEXT("Surveyor"));
    Cancellation.Production.Cancellation.ProgressPercent = 45;
    Cancellation.Production.Cancellation.RefundPercent = 75;
    Cancellation.Production.Cancellation.InvestedMatter = 50;
    Cancellation.Production.Cancellation.InvestedDawn = 20;
    Cancellation.Production.Cancellation.RefundMatter = 37;
    Cancellation.Production.Cancellation.RefundDawn = 15;
    Cancellation.Production.Cancellation.bActive = true;
    Cancellation.Production.Controls = {
        FieldControl(TEXT("BACK"),
            EEchoesFieldHudAction::ProductionCancelBack, 0),
        FieldControl(TEXT("CONFIRM CANCELLATION"),
            EEchoesFieldHudAction::ProductionCancelConfirm, 0)};
    Cancellation.Production.Controls[0].bPrimary = true;
    Widget->SetView(Cancellation);
    SelectionPanel = Widget->GetSection(EEchoesFieldHudSection::Selection);
    TestEqual(TEXT("Cancellation review exposes only Back and Confirm"),
        SelectionPanel->GetActionButtonCount(), 2);
    UEchoesFieldHudActionButton* Back = SelectionPanel->GetActionButton(0);
    UEchoesFieldHudActionButton* Confirm = SelectionPanel->GetActionButton(1);
    TestTrue(TEXT("Cancellation actions remain pointer and keyboard focusable"),
        Back != nullptr && Confirm != nullptr &&
        Back->TakeWidget()->SupportsKeyboardFocus() &&
        Confirm->TakeWidget()->SupportsKeyboardFocus() &&
        Back->GetAction() == EEchoesFieldHudAction::ProductionCancelBack &&
        Confirm->GetAction() ==
            EEchoesFieldHudAction::ProductionCancelConfirm);
    TestTrue(TEXT("Cancellation review chooses safe Back as default focus"),
        Widget->FocusDefaultAction() &&
        Widget->GetFocusedAction() ==
            EEchoesFieldHudAction::ProductionCancelBack);
    Widget->SetView(BattlefieldView(0.8f));
    SelectionPanel = Widget->GetSection(EEchoesFieldHudSection::Selection);
    UEchoesFieldHudActionButton* FirstCommand =
        Widget->GetSection(EEchoesFieldHudSection::CommandCard)
            ->GetActionButton(0);
    UTextBlock* FirstCommandLabel = FirstCommand != nullptr
        ? FirstCommand->GetPresentationLabel()
        : nullptr;
    if (!TestNotNull(TEXT("Keyboard focus target exists"), FirstCommand)) return false;
    FirstCommand->OnLostFocus.ExecuteIfBound();
    const FLinearColor RestingFill = FirstCommand->GetStyle().Normal.TintColor.GetSpecifiedColor();
    FirstCommand->OnReceivedFocus.ExecuteIfBound();
    TestTrue(TEXT("Received keyboard focus visibly changes the command tile"),
        FirstCommand->GetStyle().Normal.TintColor.GetSpecifiedColor() != RestingFill);
    FirstCommand->OnLostFocus.ExecuteIfBound();
    TestEqual(TEXT("Lost focus restores the resting command tile"),
        FirstCommand->GetStyle().Normal.TintColor.GetSpecifiedColor(), RestingFill);
    TestNotNull(TEXT("Command control owns a native UMG label"),
        FirstCommandLabel);
    if (FirstCommandLabel != nullptr)
    {
        const auto Luminance = [](const FLinearColor& C) { return .2126f*C.R + .7152f*C.G + .0722f*C.B; };
        const float TextLuminance = Luminance(FirstCommandLabel->GetColorAndOpacity().GetSpecifiedColor());
        for (const FSlateBrush* Brush : {&FirstCommand->GetStyle().Normal, &FirstCommand->GetStyle().Hovered, &FirstCommand->GetStyle().Pressed})
            TestTrue(TEXT("Enabled command states retain readable ceramic-label contrast"),
                (TextLuminance + .05f) / (Luminance(Brush->TintColor.GetSpecifiedColor()) + .05f) >= 4.5f);
        TestEqual(TEXT("Command labels honor the lower HUD scale"),
            FirstCommandLabel->GetFont().Size, 14.0f);
    }

    UEchoesFieldHudSectionWidget* ObjectivePanel =
        Widget->GetSection(EEchoesFieldHudSection::Objectives);
    const TSharedRef<SWidget> ObjectiveSlate = ObjectivePanel->TakeWidget();
    UWidget* ObjectiveRoot = ObjectivePanel->GetRootWidget();
    ObjectivePanel->SetContent(
        FText::FromString(TEXT("CHANGED SHAPE")),
        {FText::FromString(TEXT("ONE")), FText::FromString(TEXT("TWO")),
         FText::FromString(TEXT("THREE")), FText::FromString(TEXT("FOUR"))},
        {FieldControl(TEXT("ACKNOWLEDGE"),
            EEchoesFieldHudAction::ToggleTechnology, 0)},
        false,
        1.0f);
    TestTrue(TEXT("Section shape changes retain the root cached by Slate"),
        ObjectivePanel->GetRootWidget() == ObjectiveRoot &&
        &ObjectivePanel->TakeWidget().Get() == &ObjectiveSlate.Get());
    TestEqual(TEXT("Replacement content is attached beneath the stable section root"),
        ObjectivePanel->GetActionButtonCount(), 1);

    const TSharedRef<SWindow> PaintWindow = SNew(SWindow)
        .ClientSize(FVector2D(1280, 720));
    PaintWindow->SetContent(SlateWidget);
    Widget->ApplyConsoleLayout(FVector2D(1280, 720));
    UWidget* Backing = Widget->GetWidgetFromName(TEXT("ConsoleBacking"));
    auto* BackingSlot = Backing ? Cast<UCanvasPanelSlot>(Backing->Slot) : nullptr;
    auto* SelectionSlot = Cast<UCanvasPanelSlot>(SelectionPanel->Slot);
    if (!TestNotNull(TEXT("Console backing belongs to the child paint hierarchy"), BackingSlot) ||
        !TestNotNull(TEXT("Selection belongs to the console canvas"), SelectionSlot)) return false;
    TestTrue(TEXT("Console backing is ordered beneath readable content, not post-painted over it"),
        BackingSlot->GetZOrder() < SelectionSlot->GetZOrder());
    SlateWidget->SlatePrepass(1.0f);
    FHittestGrid HittestGrid;
    const FPaintArgs PaintArgs(
        &PaintWindow.Get(), HittestGrid, FVector2f::ZeroVector,
        FApp::GetCurrentTime(), FApp::GetDeltaTime());
    const FGeometry Geometry = FGeometry::MakeRoot(
        FVector2f(1280, 720), FSlateLayoutTransform());
    FSlateWindowElementList BattlefieldElements(PaintWindow);
    const int32 BattlefieldLayer = SlateWidget->Paint(
        PaintArgs, Geometry, FSlateRect(0, 0, 1280, 720),
        BattlefieldElements, 0, FWidgetStyle(), true);
    TestTrue(TEXT("Battlefield UMG hierarchy executes its real Slate paint path"),
        BattlefieldLayer > 0 && CountFieldDrawElements(
            BattlefieldElements.GetUncachedDrawElements()) > 0);

    FEchoesFieldHudView Refresh = BattlefieldView(1.5f);
    Refresh.Resources.Matter = 999;
    Refresh.Resources.SimulationTick = 78;
    Refresh.Status = FText::FromString(TEXT(
        "[SAVE_REPLAY_BINDING_FAILED] The checkpoint remains unchanged and can be retried."));
    Refresh.Subtitle = FText::FromString(TEXT(
        "Hold the reserve margin until the district ledger and the witness corridor both report a stable readback."));
    Widget->SetView(Refresh);
    TestTrue(TEXT("Dynamic resource refresh retains the attached UMG root"),
        Widget->GetRootWidget() == InitialRoot &&
        &Widget->TakeWidget().Get() == &SlateWidget.Get());
    TestEqual(TEXT("Scale-endpoint refresh retains the nine command controls"),
        Widget->GetSection(EEchoesFieldHudSection::CommandCard)
            ->GetActionButtonCount(),
        9);
    FirstCommand = Widget->GetSection(EEchoesFieldHudSection::CommandCard)
        ->GetActionButton(0);
    FirstCommandLabel = FirstCommand != nullptr
        ? FirstCommand->GetPresentationLabel()
        : nullptr;
    if (FirstCommandLabel != nullptr)
    {
        TestEqual(TEXT("Command labels honor the upper HUD scale"),
            FirstCommandLabel->GetFont().Size, 27.0f);
    }
    Widget->ApplyConsoleLayout(FVector2D(1280, 720));
    SlateWidget->SlatePrepass(1.0f);
    FSlateWindowElementList ScaleElements(PaintWindow);
    SlateWidget->Paint(
        PaintArgs, Geometry, FSlateRect(0, 0, 1280, 720),
        ScaleElements, 0, FWidgetStyle(), true);
    // A second post-layout paint gives the compact ledger its settled text
    // geometry before containment is measured.
    SlateWidget->SlatePrepass(1.0f);
    FSlateWindowElementList ResourceContainmentElements(PaintWindow);
    SlateWidget->Paint(
        PaintArgs, Geometry, FSlateRect(0, 0, 1280, 720),
        ResourceContainmentElements, 0, FWidgetStyle(), true);
    UEchoesFieldHudSectionWidget* StatusPanel =
        Widget->GetSection(EEchoesFieldHudSection::Status);
    UEchoesFieldHudSectionWidget* SubtitlePanel =
        Widget->GetSection(EEchoesFieldHudSection::Subtitle);
    TestFalse(TEXT("Transient status uses a compact non-scrolling panel"),
        StatusPanel->UsesScrollableContent());
    TestFalse(TEXT("Wrapped subtitles use a compact non-scrolling panel"),
        SubtitlePanel->UsesScrollableContent());
    const auto Console = FEchoesHudLayout::Build(FVector2D(1280, 720), 1.5f, true);
    TestTrue(TEXT("Compact global resources leave the tutorial control clear"), Console.ResourcePanel.Max.X < 1070.f);
    TestTrue(TEXT("Global resource hit bounds do not cover the central tactical target"),
        !Console.ResourcePanel.IsInsideOrOn(FEchoesHudLayout::KeyboardTargetPoint(FVector2D(1280,720),1.5f,FVector2D::ZeroVector)));

    const FVector2D MenuCenter = Console.MenuPanel.GetCenter();
    TestTrue(TEXT("Menu hit coverage is supplied by the shared HUD layout"),
        Console.bMenuVisible && Console.IsPointerOnChrome(MenuCenter));
    TestTrue(TEXT("Menu reserve is excluded from battlefield targeting"),
        !Console.IsBattlefieldPointClear(MenuCenter, FVector2D(1280, 720)));

    const FBox2D TutorialSkipBounds(FVector2D(1070.0f, 16.0f),
        FVector2D(1260.0f, 50.0f));
    TestFalse(TEXT("Tutorial skip interception cannot cover the Menu bounds"),
        Console.MenuPanel.Max.X > TutorialSkipBounds.Min.X &&
        Console.MenuPanel.Min.X < TutorialSkipBounds.Max.X &&
        Console.MenuPanel.Max.Y > TutorialSkipBounds.Min.Y &&
        Console.MenuPanel.Min.Y < TutorialSkipBounds.Max.Y);

    for (const float MenuScale : {.8f, 1.0f, 1.5f})
    {
        const FEchoesHudLayout NarrowLayout = FEchoesHudLayout::Build(
            FVector2D(800, 720), MenuScale, true);
        const bool bMenuOverlapsResources =
            NarrowLayout.MenuPanel.Max.X > NarrowLayout.ResourcePanel.Min.X &&
            NarrowLayout.MenuPanel.Min.X < NarrowLayout.ResourcePanel.Max.X &&
            NarrowLayout.MenuPanel.Max.Y > NarrowLayout.ResourcePanel.Min.Y &&
            NarrowLayout.MenuPanel.Min.Y < NarrowLayout.ResourcePanel.Max.Y;
        TestTrue(TEXT("Menu remains visible at the supported narrow viewport"),
            NarrowLayout.bMenuVisible);
        TestTrue(TEXT("Narrow menu retains shared hit coverage"),
            NarrowLayout.IsPointerOnChrome(NarrowLayout.MenuPanel.GetCenter()));
        TestTrue(TEXT("Narrow telemetry either fits or is consistently hidden"),
            !NarrowLayout.bResourceVisible || !bMenuOverlapsResources);
    }

    TestTrue(TEXT("Resource ledger uses the declared compact layout bounds"),
        ResourcePanel->GetCachedGeometry().GetLocalSize().Equals(
            Console.ResourcePanel.GetSize(), 1.0f));
    const FGeometry LedgerGeometry = ResourcePanel->GetCachedGeometry();
    const FVector2D LedgerMin = LedgerGeometry.GetAbsolutePosition();
    const FVector2D LedgerMax = LedgerMin + LedgerGeometry.GetLocalSize();
    const auto FitsLedger = [&LedgerMin, &LedgerMax](const UWidget* Child)
    {
        if (Child == nullptr)
        {
            return false;
        }
        const FGeometry ChildGeometry = Child->GetCachedGeometry();
        const FVector2D ChildMin = ChildGeometry.GetAbsolutePosition();
        const FVector2D ChildMax = ChildMin + ChildGeometry.GetLocalSize();
        return ChildMin.X >= LedgerMin.X - 1.0f && ChildMin.Y >= LedgerMin.Y - 1.0f &&
            ChildMax.X <= LedgerMax.X + 1.0f && ChildMax.Y <= LedgerMax.Y + 1.0f;
    };
    TestTrue(TEXT("Matter readout remains inside the declared resource bounds"),
        FitsLedger(ResourcePanel->GetResourceReadout(0)));
    TestTrue(TEXT("Dawn readout remains inside the declared resource bounds"),
        FitsLedger(ResourcePanel->GetResourceReadout(1)));
    TestTrue(TEXT("Logistics readout remains inside the declared resource bounds"),
        FitsLedger(ResourcePanel->GetResourceReadout(2)));
    TestTrue(TEXT("Faction identity remains inside the declared resource bounds"),
        FitsLedger(ResourcePanel->GetResourceIdentityReadout()));
    TestTrue(TEXT("Match and research context remains inside the declared resource bounds"),
        FitsLedger(ResourcePanel->GetResourceContextReadout()));
    TestTrue(TEXT("Resource monitor hit target remains inside the existing compact ledger bounds"),
        FitsLedger(ResourcePanel->GetResourceActionButton()));

    TestTrue(TEXT("Status uses the shared console geometry"),
        StatusPanel->GetCachedGeometry().GetLocalSize().Equals(Console.StatusPanel.GetSize(), 1.0f));
    TestEqual(TEXT("Standalone subtitle does not obscure battlefield; caption is in objective content"),
        SubtitlePanel->GetVisibility(), ESlateVisibility::Collapsed);
    TestTrue(TEXT("Objective controls remain present with a caption"),
        Widget->GetSection(EEchoesFieldHudSection::Objectives)->GetVisibility() == ESlateVisibility::Visible);

    for (const float ResourceScale : {.8f, 1.0f, 1.5f})
    {
        Widget->SetView(BattlefieldView(ResourceScale));
        Widget->ApplyConsoleLayout(FVector2D(1280, 720));
        SlateWidget->SlatePrepass(1.0f);
        FSlateWindowElementList ScaleContainmentElements(PaintWindow);
        SlateWidget->Paint(
            PaintArgs, Geometry, FSlateRect(0, 0, 1280, 720),
            ScaleContainmentElements, 0, FWidgetStyle(), true);
        const FEchoesHudLayout ScaleLayout = FEchoesHudLayout::Build(
            FVector2D(1280, 720), ResourceScale, true);
        const FGeometry ScaleLedgerGeometry = ResourcePanel->GetCachedGeometry();
        const FVector2D ScaleLedgerMin = ScaleLedgerGeometry.GetAbsolutePosition();
        const FVector2D ScaleLedgerMax = ScaleLedgerMin + ScaleLedgerGeometry.GetLocalSize();
        const auto FitsScaleLedger = [&ScaleLedgerMin, &ScaleLedgerMax](const UWidget* Child)
        {
            if (Child == nullptr) return false;
            const FGeometry ChildGeometry = Child->GetCachedGeometry();
            const FVector2D ChildMin = ChildGeometry.GetAbsolutePosition();
            const FVector2D ChildMax = ChildMin + ChildGeometry.GetLocalSize();
            return ChildMin.X >= ScaleLedgerMin.X - 1.0f &&
                ChildMin.Y >= ScaleLedgerMin.Y - 1.0f &&
                ChildMax.X <= ScaleLedgerMax.X + 1.0f &&
                ChildMax.Y <= ScaleLedgerMax.Y + 1.0f;
        };
        TestTrue(TEXT("Scaled resource ledger retains its declared geometry"),
            ScaleLedgerGeometry.GetLocalSize().Equals(ScaleLayout.ResourcePanel.GetSize(), 1.0f));
        TestTrue(TEXT("Scaled resource labels remain readable"),
            ResourcePanel->GetResourceLabel(0) != nullptr &&
            ResourcePanel->GetResourceLabel(0)->GetFont().Size >=
                FMath::Clamp(FMath::RoundToInt(14.0f * ResourceScale), 10, 36));
        TestTrue(TEXT("Scaled resource values remain readable"),
            ResourcePanel->GetResourceReadout(0) != nullptr &&
            ResourcePanel->GetResourceReadout(0)->GetFont().Size >=
                FMath::Clamp(FMath::RoundToInt(18.0f * ResourceScale), 10, 36));
        TestTrue(TEXT("Scaled faction context remains readable"),
            ResourcePanel->GetResourceIdentityReadout() != nullptr &&
            ResourcePanel->GetResourceIdentityReadout()->GetFont().Size >=
                FMath::Clamp(FMath::RoundToInt(14.0f * ResourceScale), 10, 36));
        TestTrue(TEXT("Scaled match and research context remains readable"),
            ResourcePanel->GetResourceContextReadout() != nullptr &&
            ResourcePanel->GetResourceContextReadout()->GetFont().Size >=
                FMath::Clamp(FMath::RoundToInt(14.0f * ResourceScale), 10, 36));
        const UWidget* ResourceChildren[] = {
            ResourcePanel->GetResourceLabel(0), ResourcePanel->GetResourceLabel(1),
            ResourcePanel->GetResourceLabel(2), ResourcePanel->GetResourceReadout(0),
            ResourcePanel->GetResourceReadout(1), ResourcePanel->GetResourceReadout(2),
            ResourcePanel->GetResourceIdentityReadout(), ResourcePanel->GetResourceContextReadout(),
            ResourcePanel->GetResourceActionButton()};
        for (const UWidget* Child : ResourceChildren)
        {
            TestTrue(FString::Printf(TEXT("Resource scale %.1f child %s contained: position %s size %s ledger %s..%s"),
                ResourceScale, *GetNameSafe(Child),
                Child ? *Child->GetCachedGeometry().GetAbsolutePosition().ToString() : TEXT("null"),
                Child ? *Child->GetCachedGeometry().GetLocalSize().ToString() : TEXT("null"),
                *ScaleLedgerMin.ToString(), *ScaleLedgerMax.ToString()), FitsScaleLedger(Child));
        }
        TestTrue(TEXT("Scaled resource strip remains clear of the tutorial skip region"),
            ScaleLayout.ResourcePanel.Max.X < 1070.0f);
    }

    FEchoesFieldHudView StatusOnly = BattlefieldView(1.0f);
    StatusOnly.bTutorialActive = true;
    StatusOnly.bObjectiveVisible = false;
    StatusOnly.Subtitle = FText::GetEmpty();
    StatusOnly.Status = FText::FromString(TEXT("Surveyor ready."));
    Widget->SetView(StatusOnly);
    Widget->ApplyConsoleLayout(FVector2D(1280, 720));
    UWidget* SkipTarget = Widget->GetWidgetFromName(TEXT("TutorialSkipHitTarget"));
    if (!TestNotNull(TEXT("Painted skip control has an actual input target"), SkipTarget))
        return false;
    FHittestGrid SkipGrid;
    SkipGrid.SetHittestArea(FVector2D::ZeroVector, FVector2D(1280, 720));
    const FPaintArgs SkipPaintArgs(&PaintWindow.Get(), SkipGrid, FVector2f::ZeroVector,
        FApp::GetCurrentTime(), FApp::GetDeltaTime());
    SlateWidget->SlatePrepass(1.0f);
    FSlateWindowElementList SkipElements(PaintWindow);
    SlateWidget->Paint(SkipPaintArgs, Geometry, FSlateRect(0, 0, 1280, 720),
        SkipElements, 0, FWidgetStyle(), true);
    const auto SkipPath = SkipGrid.GetBubblePath(FVector2D(1165, 33), 0, false);
    TestTrue(TEXT("Real Slate hit testing reaches the painted hold-to-skip rectangle"),
        SkipPath.ContainsByPredicate([&](const FWidgetAndPointer& Item)
        { return Item.Widget == SkipTarget->TakeWidget(); }));
    TestTrue(TEXT("Skip press bubbles through the HUD that owns the hold handler"),
        SkipPath.ContainsByPredicate([&](const FWidgetAndPointer& Item)
        { return Item.Widget == SlateWidget; }));
    TestTrue(TEXT("Skip input target matches the painted size and top-right position"),
        SkipTarget->GetCachedGeometry().GetLocalSize().Equals(FVector2D(190, 34), 0.1f) &&
        SkipTarget->GetCachedGeometry().LocalToAbsolute(FVector2D::ZeroVector).Equals(FVector2D(1070, 16), 0.1f));
    FEchoesFieldHudView SkipModal = StatusOnly;
    SkipModal.TutorialSkipModal.bVisible = true;
    Widget->SetView(SkipModal);
    TestEqual(TEXT("Skip hold target cannot intercept the skip-choice modal"),
        SkipTarget->GetVisibility(), ESlateVisibility::Collapsed);
    Widget->SetView(StatusOnly);
    TestEqual(TEXT("Tutorial status survives without an objective or caption"),
        Widget->GetSection(EEchoesFieldHudSection::Objectives)->GetVisibility(), ESlateVisibility::Visible);
    Widget->ApplyConsoleLayout(FVector2D(400, 360));
    TestEqual(TEXT("An unusable narrow command panel is not left over the battlefield"),
        Widget->GetSection(EEchoesFieldHudSection::CommandCard)->GetVisibility(), ESlateVisibility::Collapsed);
    Widget->ApplyConsoleLayout(FVector2D(1280, 720));
    TestEqual(TEXT("Returning to supported dimensions restores the objective/status panel"),
        Widget->GetSection(EEchoesFieldHudSection::Objectives)->GetVisibility(), ESlateVisibility::Visible);

    FEchoesFieldHudView Online;
    Online.Surface = EEchoesFieldHudSurface::OnlineFrontDoor;
    Online.Online.bVisible = true;
    Online.Online.Title = FText::FromString(TEXT("DIRECT CONNECT"));
    Online.Online.Endpoint = FText::FromString(TEXT("192.0.2.1:7777"));
    Online.Online.Controls = {
        FieldControl(TEXT("EDIT"), EEchoesFieldHudAction::OnlineEditEndpoint, 0),
        FieldControl(TEXT("JOIN"), EEchoesFieldHudAction::OnlineJoin, 0)};
    Online.Online.Controls[0].bFocused = true;
    Widget->SetView(Online);
    UEchoesFieldHudSectionWidget* OnlinePanel =
        Widget->GetSection(EEchoesFieldHudSection::OnlineFrontDoor);
    UEchoesFieldHudEndpointBox* EndpointBox = OnlinePanel->GetEndpointBox();
    TestNotNull(TEXT("Online front door owns a direct-connect editor"),
        EndpointBox);
    if (EndpointBox != nullptr)
    {
        EndpointBox->SetText(FText::FromString(TEXT("198.51.100.8:7788")));
        Online.Online.State = FText::FromString(TEXT("DISCOVERING"));
        Widget->SetView(Online);
        TestTrue(TEXT("Semantic refresh preserves uncommitted endpoint text"),
            OnlinePanel->GetEndpointBox() == EndpointBox &&
            EndpointBox->GetText().ToString() == TEXT("198.51.100.8:7788"));
    }
    UEchoesFieldHudActionButton* JoinButton =
        OnlinePanel->GetActionButton(1);
    Widget->NotifyButtonFocused(JoinButton);
    Online.Online.Controls = {
        FieldControl(TEXT("BACK"), EEchoesFieldHudAction::OnlineBack, 0),
        FieldControl(TEXT("JOIN"), EEchoesFieldHudAction::OnlineJoin, 0),
        FieldControl(TEXT("EDIT"), EEchoesFieldHudAction::OnlineEditEndpoint, 0)};
    Widget->SetView(Online);
    TestTrue(TEXT("Semantic refresh remaps focus by action identity"),
        Widget->GetFocusedAction() == EEchoesFieldHudAction::OnlineJoin &&
        Widget->GetFocusedArgument() == 0);
    Online.Online.Controls = {
        FieldControl(TEXT("EDIT"), EEchoesFieldHudAction::OnlineEditEndpoint, 0),
        FieldControl(TEXT("BACK"), EEchoesFieldHudAction::OnlineBack, 0)};
    Widget->SetView(Online);
    TestEqual(TEXT("Removed focus resolves to the safe online back action"),
        static_cast<uint8>(Widget->GetFocusedAction()),
        static_cast<uint8>(EEchoesFieldHudAction::OnlineBack));
    TestTrue(TEXT("The safe semantic default can receive modal focus"),
        Widget->FocusDefaultAction());

    FEchoesFieldHudView Campaign;
    Campaign.Surface = EEchoesFieldHudSurface::CampaignOperations;
    Campaign.HudScale = 1.5f;
    Campaign.Campaign.bVisible = true;
    Campaign.Campaign.Title = FText::FromString(TEXT("SORYN OPERATIONS"));
    Campaign.Campaign.SelectedSector = FText::FromString(TEXT("M09"));
    Campaign.Campaign.Layout.ViewportSize = FVector2D(1280, 720);
    Campaign.Campaign.Layout.ActiveMissionIndex = 8;
    FEchoesCampaignMapNode Node;
    Node.Index = 8;
    Node.ScreenPos = FVector2D(420, 280);
    Node.Radius = 18;
    Node.State = EEchoesCampaignNodeState::Available;
    Campaign.Campaign.Layout.Nodes.Add(Node);
    Campaign.Campaign.Controls = {
        FieldControl(TEXT("M09"), EEchoesFieldHudAction::CampaignSelectNode, 8),
        FieldControl(TEXT("DEPLOY"), EEchoesFieldHudAction::CampaignDeploy, 0),
        FieldControl(TEXT("BACK"), EEchoesFieldHudAction::CampaignBack, 0)};
    Widget->SetView(Campaign);
    TestEqual(TEXT("Campaign inspector exposes typed deploy and back controls"),
        Widget->GetSection(EEchoesFieldHudSection::CampaignInspector)
            ->GetActionButtonCount(),
        2);
    TestEqual(TEXT("Campaign nodes remain focusable UMG controls on the map"),
        Widget->GetCampaignMapWidget()->GetNodeButtonCount(), 1);
    UEchoesFieldHudSectionWidget* CampaignInspector =
        Widget->GetSection(EEchoesFieldHudSection::CampaignInspector);
    UEchoesFieldHudActionButton* CampaignBack =
        CampaignInspector->GetActionButton(1);
    Widget->NotifyButtonFocused(CampaignBack);
    TestTrue(TEXT("Focused actions in scrollable inspectors can be revealed"),
        CampaignInspector->ScrollActionIntoView(CampaignBack));
    TestTrue(TEXT("Campaign default focus preserves the retained back action"),
        Widget->FocusDefaultAction() &&
        Widget->GetFocusedAction() == EEchoesFieldHudAction::CampaignBack);
    FSlateWindowElementList CampaignElements(PaintWindow);
    const int32 CampaignLayer = SlateWidget->Paint(
        PaintArgs, Geometry, FSlateRect(0, 0, 1280, 720),
        CampaignElements, 0, FWidgetStyle(), true);
    TestTrue(TEXT("Campaign topology executes the real Slate geometry path"),
        CampaignLayer > 0 && CountFieldDrawElements(
            CampaignElements.GetUncachedDrawElements()) > 0);

    WorldWrapper.ForwardErrorMessages(this);
    return !HasAnyErrors();
}

#endif

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "EchoesFieldHudWidget.h"
#include "EchoesTestSaveEnvironment.h"

#include "Blueprint/UserWidget.h"
#include "Components/TextBlock.h"
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
    TestEqual(TEXT("Every semantic field panel is a modular child widget"),
        Widget->GetSectionCount(), 11);
    for (uint8 Index = 0;
         Index <= static_cast<uint8>(EEchoesFieldHudSection::Reconnect);
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
    TestEqual(TEXT("Anonymous contact has a player-scoped UMG label"),
        Widget->GetContactWidgetCount(), 1);
    TestEqual(TEXT("The tactical command card exposes all nine UMG controls"),
        Widget->GetSection(EEchoesFieldHudSection::CommandCard)
            ->GetActionButtonCount(),
        9);
    UEchoesFieldHudActionButton* FirstCommand =
        Widget->GetSection(EEchoesFieldHudSection::CommandCard)
            ->GetActionButton(0);
    UTextBlock* FirstCommandLabel = FirstCommand != nullptr
        ? Cast<UTextBlock>(FirstCommand->GetContent())
        : nullptr;
    TestNotNull(TEXT("Command control owns a native UMG label"),
        FirstCommandLabel);
    if (FirstCommandLabel != nullptr)
    {
        TestEqual(TEXT("Command labels honor the lower HUD scale"),
            FirstCommandLabel->GetFont().Size, 10.0f);
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
        ? Cast<UTextBlock>(FirstCommand->GetContent())
        : nullptr;
    if (FirstCommandLabel != nullptr)
    {
        TestEqual(TEXT("Command labels honor the upper HUD scale"),
            FirstCommandLabel->GetFont().Size, 20.0f);
    }
    SlateWidget->SlatePrepass(1.0f);
    FSlateWindowElementList ScaleElements(PaintWindow);
    SlateWidget->Paint(
        PaintArgs, Geometry, FSlateRect(0, 0, 1280, 720),
        ScaleElements, 0, FWidgetStyle(), true);
    UEchoesFieldHudSectionWidget* StatusPanel =
        Widget->GetSection(EEchoesFieldHudSection::Status);
    UEchoesFieldHudSectionWidget* SubtitlePanel =
        Widget->GetSection(EEchoesFieldHudSection::Subtitle);
    TestFalse(TEXT("Transient status uses a compact non-scrolling panel"),
        StatusPanel->UsesScrollableContent());
    TestFalse(TEXT("Wrapped subtitles use a compact non-scrolling panel"),
        SubtitlePanel->UsesScrollableContent());
    TestTrue(TEXT("Upper-scale status receives at least ten percent of a 720p field"),
        StatusPanel->GetCachedGeometry().GetLocalSize().Y >= 71.0f);
    TestTrue(TEXT("Upper-scale subtitle receives at least twelve percent of a 720p field"),
        SubtitlePanel->GetCachedGeometry().GetLocalSize().Y >= 85.0f);

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

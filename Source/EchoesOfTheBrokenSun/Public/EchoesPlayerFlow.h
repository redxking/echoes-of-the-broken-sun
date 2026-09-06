#pragma once

#include "CoreMinimal.h"

/** Local presentation navigation only; gameplay and storage retain their own authorities. */
enum class EEchoesShellScreen : uint8
{
    Gameplay, Title, Briefing, Pause, Results, Modes, Options, SaveLoad, Confirmation, Error, Credits,
    DisplayConfirmation, ReplayBrowser, ReplayTransport
};

enum class EEchoesShellAction : uint8
{
    Back, Primary, Campaign, Tutorial, Credits, Modes, NextOperation, PreviousSetting, NextSetting,
    DecreaseSetting, IncreaseSetting, Options, SaveLoad, Save, Load, Recover,
    SelectSlot, NewJourney, RestoreJourney, Resume, Restart, Concede, ReturnToMenu,
    Quit, Confirm, Cancel, Retry, ResetProfile, HudScaleDown, HudScaleUp, HighContrast,
    ReducedMotion, ReducedFlashing, EdgePan, DynamicRange,
    MasterDown, MasterUp, MusicDown, MusicUp, DialogueDown, DialogueUp,
    EffectsDown, EffectsUp, InterfaceDown, InterfaceUp, AmbienceDown, AmbienceUp,
    CameraPanDown, CameraPanUp, CameraZoomDown, CameraZoomUp,
    ResolutionPrevious, ResolutionNext, WindowMode, ApplyDisplay, KeepDisplay, RevertDisplay,
    HudScaleValue, OpenReplayBrowser, OpenReplay, ViewReplay, ReplayPlayPause,
    ReplaySpeedPrevious, ReplaySpeedNext, ReplayStep, ReplayPerspectivePrevious,
    ReplayPerspectiveNext, ReplayBookmark, ExitReplay, ReplayMapFilter, ReplayDateFilter,
    Rematch, ReplaySeek
};

struct FEchoesShellSlider
{
    FText Label;
    EEchoesShellAction Action = EEchoesShellAction::HudScaleValue;
    float Value = 1.0f;
    float Minimum = 0.8f;
    float Maximum = 1.5f;
};

struct FEchoesShellButton
{
    FText Label;
    EEchoesShellAction Action = EEchoesShellAction::Back;
    int32 Argument = 0;
    bool bEnabled = true;
};

struct FEchoesShellChartSeries
{
    FText Label;
    TArray<FVector2D> Samples;
    FLinearColor Color = FLinearColor::White;
};

/** Numeric, read-only results data. X values are elapsed seconds. */
struct FEchoesShellChart
{
    FText Title;
    FText Unit;
    TArray<FEchoesShellChartSeries> Series;
};

struct FEchoesShellView
{
    EEchoesShellScreen Screen = EEchoesShellScreen::Gameplay;
    FText Eyebrow;
    FText Title;
    FText Body;
    FText Status;
    TArray<FEchoesShellButton> Buttons;
    TArray<FEchoesShellSlider> Sliders;
    TArray<FEchoesShellChart> Charts;
    bool bHighContrast = false;
    float Scale = 1.0f;
};

/** One base screen and an explicit return stack prevent contradictory shell visibility. */
class FEchoesPlayerFlow
{
public:
    bool Is(EEchoesShellScreen Screen) const { return Base == Screen; }
    EEchoesShellScreen Current() const { return Stack.IsEmpty() ? Base : Stack.Last(); }
    EEchoesShellScreen BaseScreen() const { return Base; }
    bool HasOverlay() const { return !Stack.IsEmpty(); }
    void SetVisible(EEchoesShellScreen Screen, bool bVisible)
    {
        if (bVisible) { Base = Screen; Stack.Reset(); }
        else if (Base == Screen) { Base = EEchoesShellScreen::Gameplay; Stack.Reset(); }
    }
    void Push(EEchoesShellScreen Screen)
    {
        if (Current() != Screen) Stack.Add(Screen);
    }
    bool Back()
    {
        if (Stack.IsEmpty()) return false;
        Stack.Pop();
        return true;
    }
    void ClearOverlays() { Stack.Reset(); }
private:
    EEchoesShellScreen Base = EEchoesShellScreen::Gameplay;
    TArray<EEchoesShellScreen> Stack;
};

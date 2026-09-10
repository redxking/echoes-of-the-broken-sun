#pragma once

#include "CoreMinimal.h"

#include "EchoesTutorialCurriculumModel.h"

/** Local presentation navigation only; gameplay and storage retain their own authorities. */
enum class EEchoesShellScreen : uint8
{
    Gameplay, Title, Briefing, Pause, Results, Modes, Options, SaveLoad, Confirmation, Error, Credits,
    DisplayConfirmation, ReplayBrowser, ReplayTransport, Help,
    FeedbackHistory, Controls, ControlCapture, ResourceMonitor
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
    Rematch, ReplaySeek, Help, PracticeTutorialLesson,
    OpenFeedbackHistory, FeedbackHistoryAll, FeedbackHistoryOrders,
    FeedbackHistoryConstruction, FeedbackHistoryProduction,
    OpenControls, EditBinding, ResetBindings, CancelBinding, OpenResourceMonitor
};

/** Transient per-lesson practice state. It never owns durable profile facts. */
struct FEchoesTutorialPracticeState final
{
    /**
     * The practice gate, derived from the same constant as the completion
     * contract. Practising a lesson whose predicate does not exist can never
     * be verified, and a gate wider than the contract soft-locks completion.
     */
    static constexpr uint16 ImplementedLessonMask = EchoesTutorialLessonMask;

    [[nodiscard]] bool Begin(uint16 LessonBit)
    {
        if (LessonBit == 0 || (LessonBit & (LessonBit - 1)) != 0 ||
            (LessonBit & ImplementedLessonMask) == 0)
        {
            return false;
        }
        TargetBit = LessonBit;
        AttemptMask = 0;
        return true;
    }

    [[nodiscard]] bool RecordVerified(uint16 LessonBit)
    {
        if (TargetBit == 0 || LessonBit != TargetBit) return false;
        AttemptMask |= LessonBit;
        TargetBit = 0;
        return true;
    }

    void Reset()
    {
        TargetBit = 0;
        AttemptMask = 0;
    }

    [[nodiscard]] bool IsActive() const { return TargetBit != 0; }
    [[nodiscard]] uint16 TargetLessonBit() const { return TargetBit; }
    [[nodiscard]] uint16 VerifiedAttemptMask() const { return AttemptMask; }

private:
    uint16 TargetBit = 0;
    uint16 AttemptMask = 0;
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
    FText Section = FText::GetEmpty();
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

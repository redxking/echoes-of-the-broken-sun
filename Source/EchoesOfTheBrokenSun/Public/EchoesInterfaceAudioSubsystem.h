#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"

#include "EchoesInterfaceAudioSubsystem.generated.h"

class USoundBase;

/** Short interface feedback cues. */
UENUM()
enum class EEchoesInterfaceCue : uint8
{
    Hover,
    Select,
    Confirm,
    Reject,
    MenuOpen,
    MenuClose,
    BriefAdvance
};

/** Rate-limited player alerts. */
UENUM()
enum class EEchoesAlertCue : uint8
{
    UnderAttack,
    StructureLost,
    ProductionComplete,
    ResearchComplete,
    CapacityLow
};

/**
 * One raised terminal-threat warning, for the non-audio alert channels.
 *
 * FOG-002 requires critical alerts to reach the player through sound, text,
 * shape, and a minimap pulse. This subsystem owns only the sound channel, so
 * it records the warning here for the HUD to draw the other three. The record
 * is written even when interface audio is muted and even when the shared
 * per-class window is closed, so a terminal warning is never lost.
 *
 * Not a USTRUCT: it never crosses reflection, Blueprint, or the network.
 */
struct FEchoesTerminalAlert final
{
    /** Alert class the warning was raised under. */
    EEchoesAlertCue Alert = EEchoesAlertCue::StructureLost;
    /** World real-time seconds at which the warning was raised. */
    double RaisedSeconds = 0.0;
    /** True when the sound channel actually played it. */
    bool bAudiblePlayed = false;
    /** True when the player has dismissed it. */
    bool bAcknowledged = false;
};

/**
 * Owns interface feedback and alert playback for the local world.
 *
 * Interface cues carry a short per-cue admission window so key repeat and
 * pointer sweeps cannot stack voices. Alerts carry a longer per-class window
 * so a simultaneous-event burst produces one brief cue per class rather than
 * a siren. All twelve cues route to the interface category submix, and the
 * player's interface volume owns their output.
 *
 * Terminal threats are the documented exception. OUT-002 defines defeat as
 * the destruction of a player's final Command Core, so a Command Core loss
 * ends the match in a way that a Barracks or Dropoff loss does not. Routing
 * both through the shared StructureLost window lets an ordinary building loss
 * moments earlier swallow the only warning the player gets that the match is
 * over, which FOG-002 forbids. PlayTerminalAlert() therefore reserves against
 * its own window rather than the shared per-class one, and records the
 * warning for the HUD's text, shape, and minimap-pulse channels even when the
 * sound channel is muted.
 *
 * Presentation only: nothing here enters command validation, simulation
 * timing, visibility, saves, replays, or checksums.
 */
UCLASS()
class ECHOESOFTHEBROKENSUN_API UEchoesInterfaceAudioSubsystem final
    : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    /** Plays one interface cue; false when rate-limited, muted, or absent. */
    bool PlayInterfaceCue(EEchoesInterfaceCue Cue);

    /** Plays one alert; false when rate-limited, muted, or absent. */
    bool PlayAlert(EEchoesAlertCue Alert);

    /**
     * Raises one alert that warns of a terminal threat.
     *
     * Callers use this instead of PlayAlert() when the event ends the match
     * for the local player. OUT-002 makes the destruction of a final Command
     * Core exactly that; a Barracks, Dropoff, or utility loss is not, and
     * those keep using PlayAlert().
     *
     * Differs from PlayAlert() in three ways, all required by FOG-002:
     *  - it reserves against a dedicated window, so an ordinary loss in the
     *    shared StructureLost class moments earlier cannot swallow it, and it
     *    does not spend the shared window in return;
     *  - it records the warning for the HUD's text, shape, and minimap-pulse
     *    channels even when interface audio is muted or the cue asset is
     *    missing, so the warning survives on a channel the player can see;
     *  - it logs at Warning, so a muted, unrendered session still leaves a
     *    trace that the terminal warning was raised.
     *
     * Returns true when the warning was raised on at least one channel. That
     * is NOT the same as "a sound played" -- read GetLastTerminalAlert() and
     * its bAudiblePlayed when the caller needs the sound channel's fate.
     */
    bool PlayTerminalAlert(EEchoesAlertCue Alert);

    /** True once a terminal warning has been raised in this world. */
    [[nodiscard]] bool HasTerminalAlert() const;

    /** The most recent terminal warning; only valid when HasTerminalAlert(). */
    [[nodiscard]] const FEchoesTerminalAlert& GetLastTerminalAlert() const;

    /**
     * True when the HUD should still be drawing the terminal warning's text,
     * shape, and minimap pulse at NowSeconds (world real time).
     */
    [[nodiscard]] bool IsTerminalAlertActive(double NowSeconds) const;

    /** Marks the terminal warning dismissed; the visual channels then stop. */
    void AcknowledgeTerminalAlert();

    /** True when all twelve registered cues resolved to SoundWaves. */
    [[nodiscard]] bool HasAllAuthoredCues() const;
    [[nodiscard]] int32 GetLoadedCueCount() const;

    /** True when every loaded cue routes to the interface category submix. */
    [[nodiscard]] bool HasInterfaceSubmixRouting() const;

    /** Admission window between repeats of one interface cue. */
    [[nodiscard]] static constexpr float GetInterfaceCooldownSeconds()
    {
        return 0.06f;
    }
    /** Admission window between repeats of one alert class. */
    [[nodiscard]] static constexpr float GetAlertCooldownSeconds()
    {
        return 4.0f;
    }
    /**
     * Admission window for terminal-threat warnings, held separately from
     * every ordinary alert class. It exists only to stop two Cores lost in
     * the same splash from stacking two voices; at half a second it can never
     * suppress the *only* warning of a terminal threat, because a warning
     * that recent has already reached the player.
     */
    [[nodiscard]] static constexpr float GetTerminalAlertCooldownSeconds()
    {
        return 0.5f;
    }
    /** How long the HUD keeps drawing an unacknowledged terminal warning. */
    [[nodiscard]] static constexpr float GetTerminalAlertDisplaySeconds()
    {
        return 12.0f;
    }

    [[nodiscard]] USoundBase* ResolveInterfaceCue(
        EEchoesInterfaceCue Cue) const;
    [[nodiscard]] USoundBase* ResolveAlertCue(EEchoesAlertCue Alert) const;

#if WITH_DEV_AUTOMATION_TESTS
    bool ReserveInterfaceCueForTest(EEchoesInterfaceCue Cue, double Seconds);
    bool ReserveAlertForTest(EEchoesAlertCue Alert, double Seconds);
    void ResetRateLimitsForTest();
    [[nodiscard]] int32 GetInterfacePlayCountForTest() const
    {
        return InterfacePlayCount;
    }
    [[nodiscard]] int32 GetAlertPlayCountForTest() const
    {
        return AlertPlayCount;
    }
    /** Raises a terminal warning at an injected time, bypassing the clock. */
    bool RaiseTerminalAlertForTest(EEchoesAlertCue Alert, double Seconds);
#endif

private:
    [[nodiscard]] bool ReserveInterfaceCue(
        EEchoesInterfaceCue Cue,
        double Seconds);
    [[nodiscard]] bool ReserveAlert(EEchoesAlertCue Alert, double Seconds);
    bool RaiseTerminalAlert(EEchoesAlertCue Alert, double Seconds);
    [[nodiscard]] bool IsInterfaceAudible() const;
    [[nodiscard]] USoundBase* LoadCue(const TCHAR* Path);

    UPROPERTY(Transient)
    TMap<FName, TObjectPtr<USoundBase>> LoadedCues;

    double LastInterfaceSeconds[7] = {
        -1000.0, -1000.0, -1000.0, -1000.0, -1000.0, -1000.0, -1000.0};
    double LastAlertSeconds[5] = {
        -1000.0, -1000.0, -1000.0, -1000.0, -1000.0};
    // Held apart from LastAlertSeconds on purpose: a terminal warning must
    // not be spent by, and must not spend, an ordinary alert class's window.
    double LastTerminalSeconds = -1000.0;
    FEchoesTerminalAlert LastTerminalAlert;
    bool bHasTerminalAlert = false;
    int32 InterfacePlayCount = 0;
    int32 AlertPlayCount = 0;
};

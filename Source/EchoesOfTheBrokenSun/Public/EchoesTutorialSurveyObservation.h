#pragma once

#include "CoreMinimal.h"

/** World-centimeter geometry must be supplied by the lesson binding; no invented map targets. */
struct FEchoesTutorialSurveySetup
{
    TArray<FVector2D> Waypoints;
    FVector2D InitialCenter = FVector2D::ZeroVector;
    double InitialZoom = 0.0;
    double MinimumZoom = 0.0;
    double MaximumZoom = 0.0;
    double RequiredPanDistance = 0.0;
    double RecenterTolerance = 0.0;
};

enum class EEchoesSurveyCameraSource : uint8
{
    PlayerNavigation,
    Settled,
    Programmatic
};

/** Samples are once per authoritative 20 Hz tick, after camera input is applied.
 * The runtime binding, never a widget, must classify the source of camera movement.
 */
struct FEchoesTutorialSurveySample
{
    uint64 Session = 0;
    uint64 Tick = 0;
    FVector2D Center = FVector2D::ZeroVector;
    double Zoom = 0.0;
    EEchoesSurveyCameraSource Source = EEchoesSurveyCameraSource::Settled;
};

/** Camera predicate only. It does not award a lesson, publish narrative signals, or
 * unlock mastery: instruction delivery and Core/objective identification remain separate gates.
 */
class ECHOESOFTHEBROKENSUN_API FEchoesTutorialSurveyObservation
{
public:
    bool Begin(uint64 Session, const FEchoesTutorialSurveySetup& Setup);
    void Observe(const FEchoesTutorialSurveySample& Sample);
    void Reset();
    [[nodiscard]] bool IsActive() const { return ActiveSession != 0; }
    [[nodiscard]] bool CameraPredicateSatisfied() const;
    [[nodiscard]] int32 CompletedWaypoints() const { return WaypointIndex; }
    [[nodiscard]] int32 ConsecutiveDwellTicks() const { return DwellTicks; }

private:
    FEchoesTutorialSurveySetup Setup;
    uint64 ActiveSession = 0;
    uint64 LastTick = 0;
    FVector2D LastCenter = FVector2D::ZeroVector;
    double LastZoom = 0.0;
    bool bHasSample = false;
    bool bPanned = false;
    bool bMinimumZoomSeen = false;
    bool bMaximumZoomSeen = false;
    bool bRecentered = false;
    bool bWaypointNavigationObserved = false;
    int32 WaypointIndex = 0;
    int32 DwellTicks = 0;
};

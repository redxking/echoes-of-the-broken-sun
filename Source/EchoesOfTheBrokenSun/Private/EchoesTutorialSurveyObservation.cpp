#include "EchoesTutorialSurveyObservation.h"

namespace
{
bool FinitePoint(const FVector2D& Point)
{
    return FMath::IsFinite(Point.X) && FMath::IsFinite(Point.Y);
}
}

void FEchoesTutorialSurveyObservation::Reset()
{
    *this = FEchoesTutorialSurveyObservation{};
}

bool FEchoesTutorialSurveyObservation::Begin(uint64 Session, const FEchoesTutorialSurveySetup& InSetup)
{
    Reset();
    if (Session == 0 || InSetup.Waypoints.Num() != 3 || !FinitePoint(InSetup.InitialCenter) ||
        !FMath::IsFinite(InSetup.MinimumZoom) || !FMath::IsFinite(InSetup.MaximumZoom) ||
        !FMath::IsFinite(InSetup.InitialZoom) || !FMath::IsFinite(InSetup.RequiredPanDistance) ||
        !FMath::IsFinite(InSetup.RecenterTolerance) || InSetup.MinimumZoom <= 0.0 ||
        InSetup.MaximumZoom <= InSetup.MinimumZoom || InSetup.InitialZoom < InSetup.MinimumZoom ||
        InSetup.InitialZoom > InSetup.MaximumZoom || InSetup.RecenterTolerance <= 0.0 ||
        InSetup.RequiredPanDistance <= InSetup.RecenterTolerance)
        return false;
    for (int32 Index = 0; Index < InSetup.Waypoints.Num(); ++Index)
    {
        if (!FinitePoint(InSetup.Waypoints[Index])) return false;
        for (int32 Prior = 0; Prior < Index; ++Prior)
            if (FVector2D::Distance(InSetup.Waypoints[Index], InSetup.Waypoints[Prior]) <= 400.0)
                return false; // A stationary camera must not satisfy two overlapping targets.
    }
    Setup = InSetup;
    ActiveSession = Session;
    LastCenter = Setup.InitialCenter;
    LastZoom = Setup.InitialZoom;
    return true;
}

void FEchoesTutorialSurveyObservation::Observe(const FEchoesTutorialSurveySample& Sample)
{
    if (!IsActive() || Sample.Session != ActiveSession) return;
    if (bHasSample && Sample.Tick <= LastTick) return; // Duplicate and stale ticks earn nothing.
    if (!FinitePoint(Sample.Center) || !FMath::IsFinite(Sample.Zoom) ||
        Sample.Zoom < Setup.MinimumZoom || Sample.Zoom > Setup.MaximumZoom ||
        (Sample.Source != EEchoesSurveyCameraSource::PlayerNavigation &&
            Sample.Source != EEchoesSurveyCameraSource::Settled))
    {
        Reset(); // Teleports/cinematics/restores require a fresh lesson observation attempt.
        return;
    }
    if (bHasSample && Sample.Tick - LastTick != 1) DwellTicks = 0;
    const bool bMoved = !Sample.Center.Equals(LastCenter, UE_DOUBLE_SMALL_NUMBER);
    const bool bZoomed = !FMath::IsNearlyEqual(Sample.Zoom, LastZoom, UE_DOUBLE_SMALL_NUMBER);
    if (Sample.Source == EEchoesSurveyCameraSource::Settled && (bMoved || bZoomed))
    {
        Reset(); // Unattributed camera changes cannot masquerade as player navigation.
        return;
    }
    if (Sample.Source == EEchoesSurveyCameraSource::PlayerNavigation)
    {
        if (bMoved)
        {
            bWaypointNavigationObserved = true;
            bPanned |= FVector2D::Distance(Sample.Center, Setup.InitialCenter) >= Setup.RequiredPanDistance;
            bRecentered |= bPanned && FVector2D::Distance(Sample.Center, Setup.InitialCenter) <= Setup.RecenterTolerance;
        }
        if (bZoomed)
        {
            bMinimumZoomSeen |= FMath::IsNearlyEqual(Sample.Zoom, Setup.MinimumZoom, UE_DOUBLE_SMALL_NUMBER);
            bMaximumZoomSeen |= FMath::IsNearlyEqual(Sample.Zoom, Setup.MaximumZoom, UE_DOUBLE_SMALL_NUMBER);
        }
    }
    if (WaypointIndex < Setup.Waypoints.Num())
    {
        if (bWaypointNavigationObserved && FVector2D::Distance(Sample.Center, Setup.Waypoints[WaypointIndex]) <= 200.0)
        {
            if (++DwellTicks == 30)
            {
                ++WaypointIndex;
                DwellTicks = 0;
                bWaypointNavigationObserved = false;
            }
        }
        else DwellTicks = 0;
    }
    LastTick = Sample.Tick;
    LastCenter = Sample.Center;
    LastZoom = Sample.Zoom;
    bHasSample = true;
}

bool FEchoesTutorialSurveyObservation::CameraPredicateSatisfied() const
{
    return IsActive() && bPanned && bMinimumZoomSeen && bMaximumZoomSeen && bRecentered && WaypointIndex == 3;
}

#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "EchoesTutorialSurveyObservation.h"
#include <limits>

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEchoesTutorialSurveyObservationTest,
    "Echoes.Runtime.Campaign.TutorialSurveyObservation",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::EngineFilter)

bool FEchoesTutorialSurveyObservationTest::RunTest(const FString& Parameters)
{
    (void)Parameters;
    FEchoesTutorialSurveySetup Setup;
    Setup.Waypoints = {{1000, 0}, {2000, 0}, {3000, 0}};
    Setup.MinimumZoom = 1400;
    Setup.MaximumZoom = 6200;
    Setup.InitialZoom = 3000;
    Setup.RequiredPanDistance = 500;
    Setup.RecenterTolerance = 100;
    FEchoesTutorialSurveyObservation Observer;
    TestTrue(TEXT("Authored geometry starts a camera observation attempt"), Observer.Begin(7, Setup));
    FEchoesTutorialSurveySample Sample;
    Sample.Session = 7;
    Sample.Zoom = 3000;
    for (int32 Tick = 0; Tick < 100; ++Tick)
    {
        Sample.Tick = Tick;
        Observer.Observe(Sample);
    }
    TestFalse(TEXT("Elapsed time alone cannot satisfy camera navigation"), Observer.CameraPredicateSatisfied());
    Sample.Source = EEchoesSurveyCameraSource::PlayerNavigation;
    Sample.Center = Setup.Waypoints[0];
    Sample.Tick = 100;
    Observer.Observe(Sample);
    for (int32 Duplicate = 0; Duplicate < 50; ++Duplicate) Observer.Observe(Sample);
    TestEqual(TEXT("Duplicate ticks cannot accumulate dwell"), Observer.ConsecutiveDwellTicks(), 1);
    Sample.Source = EEchoesSurveyCameraSource::Settled;
    Sample.Tick = 102;
    Observer.Observe(Sample);
    TestEqual(TEXT("Missing sample breaks consecutive dwell"), Observer.ConsecutiveDwellTicks(), 1);
    for (int32 Tick = 103; Tick < 131; ++Tick)
    {
        Sample.Tick = Tick;
        Observer.Observe(Sample);
    }
    TestEqual(TEXT("29 ticks do not complete a waypoint"), Observer.CompletedWaypoints(), 0);
    Sample.Tick = 131;
    Observer.Observe(Sample);
    TestEqual(TEXT("30 consecutive ticks complete one waypoint"), Observer.CompletedWaypoints(), 1);
    for (int32 Index = 1; Index < 3; ++Index)
    {
        Sample.Source = EEchoesSurveyCameraSource::PlayerNavigation;
        Sample.Center = Setup.Waypoints[Index];
        for (int32 Dwell = 0; Dwell < 30; ++Dwell)
        {
            ++Sample.Tick;
            Observer.Observe(Sample);
            Sample.Source = EEchoesSurveyCameraSource::Settled;
        }
    }
    TestEqual(TEXT("Three separate target dwells are observed"), Observer.CompletedWaypoints(), 3);
    TestFalse(TEXT("Waypoint completion does not waive zoom and recenter"), Observer.CameraPredicateSatisfied());
    Sample.Source = EEchoesSurveyCameraSource::PlayerNavigation;
    Sample.Zoom = Setup.MinimumZoom;
    ++Sample.Tick;
    Observer.Observe(Sample);
    Sample.Zoom = Setup.MaximumZoom;
    ++Sample.Tick;
    Observer.Observe(Sample);
    TestFalse(TEXT("Both zoom bounds still require recenter after pan"), Observer.CameraPredicateSatisfied());
    Sample.Center = Setup.InitialCenter;
    ++Sample.Tick;
    Observer.Observe(Sample);
    TestTrue(TEXT("Measured pan, zoom, recenter and all dwells satisfy camera predicate"), Observer.CameraPredicateSatisfied());
    Sample.Session = 6;
    Sample.Source = EEchoesSurveyCameraSource::Programmatic;
    ++Sample.Tick;
    Observer.Observe(Sample);
    TestTrue(TEXT("An old session cannot mutate current progress"), Observer.CameraPredicateSatisfied());
    Sample.Session = 7;
    Observer.Observe(Sample);
    TestFalse(TEXT("Programmatic reposition invalidates the attempt"), Observer.IsActive());
    TestTrue(TEXT("Retry starts clean"), Observer.Begin(8, Setup));
    TestFalse(TEXT("Retry does not retain completion"), Observer.CameraPredicateSatisfied());
    TestEqual(TEXT("Retry clears waypoints"), Observer.CompletedWaypoints(), 0);
    Sample.Session = 8;
    Sample.Source = EEchoesSurveyCameraSource::Settled;
    Sample.Center = Setup.Waypoints[0];
    Sample.Zoom = Setup.InitialZoom;
    Observer.Observe(Sample);
    TestFalse(TEXT("Unattributed movement invalidates the attempt"), Observer.IsActive());
    Observer.Begin(9, Setup);
    Sample.Session = 9;
    Sample.Source = EEchoesSurveyCameraSource::PlayerNavigation;
    Sample.Zoom = std::numeric_limits<double>::quiet_NaN();
    Observer.Observe(Sample);
    TestFalse(TEXT("Nonfinite geometry cannot award progress"), Observer.IsActive());
    Observer.Begin(12, Setup);
    Sample.Session = 12;
    Sample.Zoom = Setup.InitialZoom;
    Sample.Center = Setup.Waypoints[0];
    Observer.Observe(Sample);
    TestEqual(TEXT("Player movement starts dwell"), Observer.ConsecutiveDwellTicks(), 1);
    Sample.Source = static_cast<EEchoesSurveyCameraSource>(255);
    ++Sample.Tick;
    Observer.Observe(Sample);
    TestFalse(TEXT("Unknown camera source invalidates the attempt"), Observer.IsActive());
    Observer.Begin(13, Setup);
    Sample.Session = 13;
    Sample.Source = EEchoesSurveyCameraSource::PlayerNavigation;
    Sample.Center = Setup.Waypoints[1];
    ++Sample.Tick;
    Observer.Observe(Sample);
    TestEqual(TEXT("Out-of-order waypoint earns no dwell"), Observer.ConsecutiveDwellTicks(), 0);
    Sample.Center = Setup.Waypoints[0] + FVector2D(200, 0);
    ++Sample.Tick;
    Observer.Observe(Sample);
    TestEqual(TEXT("200 cm boundary counts"), Observer.ConsecutiveDwellTicks(), 1);
    Sample.Center.X += 1;
    ++Sample.Tick;
    Observer.Observe(Sample);
    TestEqual(TEXT("Leaving the 200 cm radius resets dwell"), Observer.ConsecutiveDwellTicks(), 0);
    TestFalse(TEXT("Zero session is invalid"), Observer.Begin(0, Setup));
    Setup.Waypoints[1] = Setup.Waypoints[0];
    TestFalse(TEXT("Overlapping target geometry is rejected"), Observer.Begin(10, Setup));
    Setup.Waypoints.Pop();
    TestFalse(TEXT("Exactly three authored targets are required"), Observer.Begin(11, Setup));
    return true;
}
#endif

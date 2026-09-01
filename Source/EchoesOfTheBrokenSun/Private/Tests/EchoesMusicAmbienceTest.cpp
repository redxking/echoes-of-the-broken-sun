#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "EchoesTestSaveEnvironment.h"

#include "Components/AudioComponent.h"
#include "EchoesAmbienceSubsystem.h"
#include "EchoesAudioMixSubsystem.h"
#include "EchoesMusicSubsystem.h"
#include "Engine/World.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundSubmix.h"
#include "Tests/AutomationCommon.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEchoesMusicAmbienceTest,
    "Echoes.Runtime.Audio.MusicAmbience",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::ClientContext |
        EAutomationTestFlags::EngineFilter)

bool FEchoesMusicAmbienceTest::RunTest(const FString& Parameters)
{
    (void)Parameters;

    FEchoesScopedTestSaveEnvironment TestSaveEnvironment(*this);
    if (!TestSaveEnvironment.IsReady())
    {
        return false;
    }
    FTestWorldWrapper WorldWrapper;
    if (!WorldWrapper.CreateTestWorld(EWorldType::Game))
    {
        WorldWrapper.ForwardErrorMessages(this);
        AddError(TEXT("Could not create the music/ambience test world."));
        return false;
    }
    UWorld* World = WorldWrapper.GetTestWorld();
    UEchoesMusicSubsystem* Music =
        World != nullptr ? World->GetSubsystem<UEchoesMusicSubsystem>()
                         : nullptr;
    UEchoesAmbienceSubsystem* Ambience =
        World != nullptr ? World->GetSubsystem<UEchoesAmbienceSubsystem>()
                         : nullptr;
    if (!TestNotNull(TEXT("Music subsystem is available"), Music) ||
        !TestNotNull(TEXT("Ambience subsystem is available"), Ambience))
    {
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    // --- Registered cues load and route to their category submixes ---------

    TestTrue(TEXT("All fifteen registered music cues load"),
             Music->HasAllAuthoredCues());
    TestEqual(TEXT("Music cue count is fifteen"),
              Music->GetLoadedCueCount(),
              15);
    TestTrue(TEXT("Every music cue's base submix is the music submix"),
             Music->HasMusicSubmixRouting());
    TestTrue(TEXT("All five registered ambience cues load"),
             Ambience->HasAllAuthoredCues());
    TestEqual(TEXT("Ambience cue count is five"),
              Ambience->GetLoadedCueCount(),
              5);
    TestTrue(TEXT("Every ambience cue's base submix is the ambience submix"),
             Ambience->HasAmbienceSubmixRouting());

    // --- Music context selection -------------------------------------------

    TestEqual(TEXT("Music starts silent"),
              static_cast<uint8>(Music->GetMusicContext()),
              static_cast<uint8>(EEchoesMusicContext::Silent));
    TestNull(TEXT("Silent resolves no bed"),
             Music->ResolveBedCue(
                 EEchoesMusicContext::Silent,
                 echoes::sim::Faction::MeridianCompact,
                 1));

    USoundBase* TitleCue = Music->ResolveBedCue(
        EEchoesMusicContext::Title,
        echoes::sim::Faction::MeridianCompact,
        1);
    TestNotNull(TEXT("Title resolves the title theme"), TitleCue);

    USoundBase* MeridianCue = Music->ResolveBedCue(
        EEchoesMusicContext::FactionTheme,
        echoes::sim::Faction::MeridianCompact,
        1);
    USoundBase* KharuunCue = Music->ResolveBedCue(
        EEchoesMusicContext::FactionTheme,
        echoes::sim::Faction::KharuunAssemblies,
        1);
    USoundBase* ChoirCue = Music->ResolveBedCue(
        EEchoesMusicContext::FactionTheme,
        echoes::sim::Faction::HollowChoir,
        1);
    TestTrue(TEXT("The three faction themes are three distinct cues"),
             MeridianCue != nullptr && KharuunCue != nullptr &&
                 ChoirCue != nullptr && MeridianCue != KharuunCue &&
                 KharuunCue != ChoirCue && MeridianCue != ChoirCue);

    USoundBase* ActI = Music->ResolveBedCue(
        EEchoesMusicContext::ActBed,
        echoes::sim::Faction::MeridianCompact,
        1);
    USoundBase* ActII = Music->ResolveBedCue(
        EEchoesMusicContext::ActBed,
        echoes::sim::Faction::MeridianCompact,
        2);
    USoundBase* ActIII = Music->ResolveBedCue(
        EEchoesMusicContext::ActBed,
        echoes::sim::Faction::MeridianCompact,
        3);
    TestTrue(TEXT("The three act beds are three distinct cues"),
             ActI != nullptr && ActII != nullptr && ActIII != nullptr &&
                 ActI != ActII && ActII != ActIII && ActI != ActIII);

    // --- Context changes play and crossfade rather than hard-cut -----------

    Music->SetMusicContext(EEchoesMusicContext::Title);
    UAudioComponent* TitleComponent = Music->GetBedComponentForTest();
    TestNotNull(TEXT("Entering the title context starts a bed"),
                TitleComponent);
    if (TitleComponent != nullptr)
    {
        TestTrue(TEXT("The title bed is playing"),
                 TitleComponent->IsPlaying());
        TestTrue(TEXT("The title bed plays the title cue"),
                 TitleComponent->Sound == TitleCue);
    }

    Music->SetMusicContext(
        EEchoesMusicContext::FactionTheme,
        echoes::sim::Faction::KharuunAssemblies);
    UAudioComponent* FactionComponent = Music->GetBedComponentForTest();
    TestNotNull(TEXT("Switching context starts the new bed"),
                FactionComponent);
    TestTrue(TEXT("The new bed is a different component"),
             FactionComponent != TitleComponent);
    if (FactionComponent != nullptr)
    {
        TestTrue(TEXT("The new bed plays the Kharuun theme"),
                 FactionComponent->Sound == KharuunCue);
    }
    TestTrue(TEXT("The crossfade window is positive — no hard cut"),
             UEchoesMusicSubsystem::GetCrossfadeSeconds() > 0.0f);
    if (TitleComponent != nullptr)
    {
        // The outgoing bed is still fading out inside the crossfade window
        // rather than stopped dead on the same frame.
        TestTrue(TEXT("The outgoing bed is still fading, not cut"),
                 TitleComponent->IsPlaying());
    }

    // Re-selecting the same context is a no-op, not a restart.
    Music->SetMusicContext(
        EEchoesMusicContext::FactionTheme,
        echoes::sim::Faction::KharuunAssemblies);
    TestTrue(TEXT("Re-selecting the same context keeps the same bed"),
             Music->GetBedComponentForTest() == FactionComponent);

    // --- Threat layers ------------------------------------------------------

    TestFalse(TEXT("Tension starts inactive"), Music->IsTensionLayerActive());
    TestFalse(TEXT("Combat starts inactive"), Music->IsCombatLayerActive());
    Music->SetThreatLayers(true, false);
    TestTrue(TEXT("Tension can rise alone"), Music->IsTensionLayerActive());
    TestFalse(TEXT("Combat stays down"), Music->IsCombatLayerActive());
    UAudioComponent* TensionComponent = Music->GetTensionComponentForTest();
    TestNotNull(TEXT("The tension layer is playing"), TensionComponent);
    TestNull(TEXT("No combat component exists yet"),
             Music->GetCombatComponentForTest());

    Music->SetThreatLayers(true, true);
    UAudioComponent* CombatComponent = Music->GetCombatComponentForTest();
    TestNotNull(TEXT("The combat layer joins without dropping tension"),
                CombatComponent);
    TestTrue(TEXT("Tension remains active under combat"),
             Music->IsTensionLayerActive());
    TestTrue(TEXT("The bed continues under both layers"),
             Music->GetBedComponentForTest() == FactionComponent);

    Music->SetThreatLayers(false, false);
    TestFalse(TEXT("Both layers lower"),
              Music->IsTensionLayerActive() || Music->IsCombatLayerActive());

    // --- Stingers -----------------------------------------------------------

    TestNotNull(TEXT("Victory resolves a cue"),
                Music->ResolveStingerCue(EEchoesMusicStinger::Victory));
    TestNotNull(TEXT("Defeat resolves a cue"),
                Music->ResolveStingerCue(EEchoesMusicStinger::Defeat));
    TSet<USoundBase*> EndingCues;
    EndingCues.Add(Music->ResolveStingerCue(
        EEchoesMusicStinger::EndingRestoration));
    EndingCues.Add(Music->ResolveStingerCue(
        EEchoesMusicStinger::EndingStabilization));
    EndingCues.Add(Music->ResolveStingerCue(
        EEchoesMusicStinger::EndingExtinguishment));
    EndingCues.Add(Music->ResolveStingerCue(
        EEchoesMusicStinger::EndingOpenEvolution));
    EndingCues.Remove(nullptr);
    TestEqual(TEXT("The four ending stingers are four distinct cues"),
              EndingCues.Num(),
              4);
    const int32 StingersBefore = Music->GetStingerPlayCountForTest();
    TestTrue(TEXT("A stinger reaches playback"),
             Music->PlayStinger(EEchoesMusicStinger::Victory));
    TestEqual(TEXT("The stinger playback was recorded"),
              Music->GetStingerPlayCountForTest(),
              StingersBefore + 1);

    // --- Ambience beds ------------------------------------------------------

    TestEqual(TEXT("Ambience starts with no bed"),
              static_cast<uint8>(Ambience->GetAmbienceBed()),
              static_cast<uint8>(EEchoesAmbienceBed::None));
    TSet<USoundBase*> BedCues;
    BedCues.Add(Ambience->ResolveBedCue(EEchoesAmbienceBed::GlassScar));
    BedCues.Add(Ambience->ResolveBedCue(EEchoesAmbienceBed::LumeReach));
    BedCues.Add(Ambience->ResolveBedCue(EEchoesAmbienceBed::ArkCity));
    BedCues.Add(Ambience->ResolveBedCue(EEchoesAmbienceBed::Crownfall));
    BedCues.Remove(nullptr);
    TestEqual(TEXT("The four site beds are four distinct cues"),
              BedCues.Num(),
              4);

    Ambience->SetAmbienceBed(EEchoesAmbienceBed::GlassScar);
    UAudioComponent* GlassBed = Ambience->GetBedComponentForTest();
    TestNotNull(TEXT("Selecting the Glass Scar starts its bed"), GlassBed);
    if (GlassBed != nullptr)
    {
        TestTrue(TEXT("The Glass Scar bed is playing"),
                 GlassBed->IsPlaying());
        TestTrue(TEXT("The bed plays the Glass Scar cue"),
                 GlassBed->Sound ==
                     Ambience->ResolveBedCue(EEchoesAmbienceBed::GlassScar));
    }
    Ambience->SetAmbienceBed(EEchoesAmbienceBed::Crownfall);
    UAudioComponent* CrownfallBed = Ambience->GetBedComponentForTest();
    TestTrue(TEXT("Changing site crossfades to a new bed component"),
             CrownfallBed != nullptr && CrownfallBed != GlassBed);
    if (GlassBed != nullptr)
    {
        TestTrue(TEXT("The outgoing site bed fades rather than cutting"),
                 GlassBed->IsPlaying());
    }

    TestFalse(TEXT("The Well layer starts inactive"),
              Ambience->IsWellLayerActive());
    Ambience->SetWellProximity(true);
    TestTrue(TEXT("Well proximity raises the Well layer"),
             Ambience->IsWellLayerActive());
    TestNotNull(TEXT("The Well layer is playing"),
                Ambience->GetWellComponentForTest());
    TestTrue(TEXT("The site bed continues under the Well layer"),
             Ambience->GetBedComponentForTest() == CrownfallBed);
    Ambience->SetWellProximity(false);
    TestFalse(TEXT("Leaving Well proximity lowers the layer"),
              Ambience->IsWellLayerActive());

    return true;
}

#endif

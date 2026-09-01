#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "EchoesTestSaveEnvironment.h"

#include "EchoesAudioMix.h"
#include "EchoesAudioMixSubsystem.h"
#include "EchoesGameUserSettings.h"
#include "Engine/World.h"
#include "Sound/SoundSubmix.h"
#include "Tests/AutomationCommon.h"

#include <limits>

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEchoesAudioMixTest,
    "Echoes.Runtime.Audio.MixArchitecture",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::ClientContext |
        EAutomationTestFlags::EngineFilter)

namespace
{
/**
 * A deliberately uneven volume set. Every category sits at a different
 * position, so a routing mistake that reads the wrong control cannot hide
 * behind two categories that happen to share a value.
 */
FEchoesAudioMixVolumes DistinctVolumes()
{
    FEchoesAudioMixVolumes Volumes;
    Volumes.Master = 1.0f;
    Volumes.Music = 0.90f;
    Volumes.Dialogue = 0.80f;
    Volumes.Interface = 0.55f;
    Volumes.Ambience = 0.35f;
    Volumes.Effects = 0.20f;
    return Volumes;
}
}

bool FEchoesAudioMixTest::RunTest(const FString& Parameters)
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
        AddError(TEXT("Could not create the audio-mix test world."));
        return false;
    }
    UWorld* World = WorldWrapper.GetTestWorld();
    UEchoesAudioMixSubsystem* Mix =
        World != nullptr ? World->GetSubsystem<UEchoesAudioMixSubsystem>()
                         : nullptr;
    if (!TestNotNull(TEXT("Audio-mix subsystem is available"), Mix))
    {
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    // --- The graph itself -------------------------------------------------

    TestTrue(TEXT("The mix graph has a master and five category submixes"),
             Mix->HasCompleteGraph());
    TestTrue(TEXT("Every category submix is a child of the master submix"),
             Mix->HasMasterRouting());
    TestEqual(TEXT("The graph carries exactly five categories"),
              EchoesAudioCategoryCount,
              5);

    TSet<const USoundSubmix*> DistinctSubmixes;
    for (const EEchoesAudioCategory Category : EchoesAudioCategories)
    {
        USoundSubmix* Submix = Mix->GetCategorySubmix(Category);
        if (!TestNotNull(
                *FString::Printf(
                    TEXT("Category %s has a submix"),
                    EchoesAudioMix::CategoryStableName(Category)),
                Submix))
        {
            WorldWrapper.ForwardErrorMessages(this);
            return false;
        }
        TestTrue(
            *FString::Printf(
                TEXT("Category submix %s is subsystem-owned"),
                EchoesAudioMix::CategoryStableName(Category)),
            Submix->GetOuter() == Mix);
        TestTrue(
            *FString::Printf(
                TEXT("Category submix %s is transient"),
                EchoesAudioMix::CategoryStableName(Category)),
            Submix->HasAnyFlags(RF_Transient));
        DistinctSubmixes.Add(Submix);
    }
    TestEqual(TEXT("The five categories use five distinct submixes"),
              DistinctSubmixes.Num(),
              EchoesAudioCategoryCount);
    TestFalse(TEXT("The master submix is not itself a category submix"),
              DistinctSubmixes.Contains(Mix->GetMasterSubmix()));

    // --- Each control moves its own category and no other -----------------

    const FEchoesAudioMixVolumes Baseline = DistinctVolumes();
    Mix->ApplyVolumes(Baseline, false);
    float BaselineGains[EchoesAudioCategoryCount] = {};
    for (const EEchoesAudioCategory Category : EchoesAudioCategories)
    {
        BaselineGains[EchoesAudioMix::CategoryIndex(Category)] =
            Mix->GetAppliedCategoryGain(Category);
    }
    for (const EEchoesAudioCategory Moved : EchoesAudioCategories)
    {
        FEchoesAudioMixVolumes Changed = Baseline;
        const float MovedTo = 0.05f;
        switch (Moved)
        {
            case EEchoesAudioCategory::Music:
                Changed.Music = MovedTo;
                break;
            case EEchoesAudioCategory::Dialogue:
                Changed.Dialogue = MovedTo;
                break;
            case EEchoesAudioCategory::Interface:
                Changed.Interface = MovedTo;
                break;
            case EEchoesAudioCategory::Ambience:
                Changed.Ambience = MovedTo;
                break;
            case EEchoesAudioCategory::Effects:
                Changed.Effects = MovedTo;
                break;
        }
        Mix->ApplyVolumes(Changed, false);
        for (const EEchoesAudioCategory Observed : EchoesAudioCategories)
        {
            const int32 Index = EchoesAudioMix::CategoryIndex(Observed);
            const float Now = Mix->GetAppliedCategoryGain(Observed);
            const FString Label = FString::Printf(
                TEXT("Moving %s changes %s"),
                EchoesAudioMix::CategoryStableName(Moved),
                EchoesAudioMix::CategoryStableName(Observed));
            if (Observed == Moved)
            {
                TestTrue(*Label, !FMath::IsNearlyEqual(Now, BaselineGains[Index]));
                TestTrue(
                    *FString::Printf(
                        TEXT("Lowering %s lowers its own output"),
                        EchoesAudioMix::CategoryStableName(Moved)),
                    Now < BaselineGains[Index]);
            }
            else
            {
                TestTrue(
                    *FString::Printf(
                        TEXT("Moving %s leaves %s untouched"),
                        EchoesAudioMix::CategoryStableName(Moved),
                        EchoesAudioMix::CategoryStableName(Observed)),
                    FMath::IsNearlyEqual(Now, BaselineGains[Index]));
            }
        }
    }

    // --- Reduced dynamic range operates across the whole graph ------------

    Mix->ApplyVolumes(Baseline, false);
    const float StandardSpread = Mix->GetAppliedGainSpread();
    float StandardGains[EchoesAudioCategoryCount] = {};
    for (const EEchoesAudioCategory Category : EchoesAudioCategories)
    {
        StandardGains[EchoesAudioMix::CategoryIndex(Category)] =
            Mix->GetAppliedCategoryGain(Category);
    }
    Mix->ApplyVolumes(Baseline, true);
    const float ReducedSpread = Mix->GetAppliedGainSpread();
    TestTrue(TEXT("Reduced dynamic range narrows the spread across the graph"),
             ReducedSpread < StandardSpread);
    for (const EEchoesAudioCategory Category : EchoesAudioCategories)
    {
        const int32 Index = EchoesAudioMix::CategoryIndex(Category);
        TestTrue(
            *FString::Printf(
                TEXT("Reduced dynamic range reaches the %s bus, not effects alone"),
                EchoesAudioMix::CategoryStableName(Category)),
            !FMath::IsNearlyEqual(
                Mix->GetAppliedCategoryGain(Category),
                StandardGains[Index]));
        TestTrue(
            *FString::Printf(
                TEXT("Reduced %s output stays within unit gain"),
                EchoesAudioMix::CategoryStableName(Category)),
            Mix->GetAppliedCategoryGain(Category) >= 0.0f &&
                Mix->GetAppliedCategoryGain(Category) <= 1.0f);
    }
    TestTrue(TEXT("Reduced dynamic range lifts the quietest category"),
             EchoesAudioMix::ResolveCategoryGain(
                 Baseline, EEchoesAudioCategory::Effects, true) >
                 EchoesAudioMix::ResolveCategoryGain(
                     Baseline, EEchoesAudioCategory::Effects, false));
    TestTrue(TEXT("Reduced dynamic range lowers the loudest category"),
             EchoesAudioMix::ResolveCategoryGain(
                 Baseline, EEchoesAudioCategory::Music, true) <
                 EchoesAudioMix::ResolveCategoryGain(
                     Baseline, EEchoesAudioCategory::Music, false));

    // --- Mute and master behaviour ----------------------------------------

    FEchoesAudioMixVolumes Muted = Baseline;
    Muted.Ambience = 0.0f;
    Mix->ApplyVolumes(Muted, true);
    TestTrue(TEXT("A muted category stays silent under reduced dynamic range"),
             Mix->GetAppliedCategoryGain(EEchoesAudioCategory::Ambience) ==
                 0.0f);
    TestTrue(TEXT("Muting one category leaves the others audible"),
             Mix->GetAppliedCategoryGain(EEchoesAudioCategory::Music) > 0.0f);

    FEchoesAudioMixVolumes MasterHalved = Baseline;
    MasterHalved.Master = 0.5f;
    Mix->ApplyVolumes(MasterHalved, false);
    for (const EEchoesAudioCategory Category : EchoesAudioCategories)
    {
        const int32 Index = EchoesAudioMix::CategoryIndex(Category);
        TestTrue(
            *FString::Printf(
                TEXT("Master scales %s linearly"),
                EchoesAudioMix::CategoryStableName(Category)),
            FMath::IsNearlyEqual(
                Mix->GetAppliedCategoryGain(Category),
                StandardGains[Index] * 0.5f,
                KINDA_SMALL_NUMBER));
    }

    FEchoesAudioMixVolumes MasterMuted = Baseline;
    MasterMuted.Master = 0.0f;
    Mix->ApplyVolumes(MasterMuted, true);
    for (const EEchoesAudioCategory Category : EchoesAudioCategories)
    {
        TestTrue(
            *FString::Printf(
                TEXT("A muted master silences %s"),
                EchoesAudioMix::CategoryStableName(Category)),
            Mix->GetAppliedCategoryGain(Category) == 0.0f);
    }

    // --- The mix fails closed on unusable input ---------------------------

    FEchoesAudioMixVolumes Invalid = Baseline;
    Invalid.Music = std::numeric_limits<float>::quiet_NaN();
    Invalid.Dialogue = 4.0f;
    Invalid.Interface = -3.0f;
    Mix->ApplyVolumes(Invalid, false);
    TestTrue(TEXT("A non-finite category volume resolves to silence"),
             Mix->GetAppliedCategoryGain(EEchoesAudioCategory::Music) == 0.0f);
    TestTrue(TEXT("An over-unit category volume is clamped to unit gain"),
             FMath::IsNearlyEqual(
                 Mix->GetAppliedCategoryGain(EEchoesAudioCategory::Dialogue),
                 1.0f));
    TestTrue(TEXT("A negative category volume resolves to silence"),
             Mix->GetAppliedCategoryGain(EEchoesAudioCategory::Interface) ==
                 0.0f);

    // --- The player's own settings drive the graph ------------------------

    UEchoesGameUserSettings* Settings = UEchoesGameUserSettings::Get();
    if (!TestNotNull(TEXT("Player audio settings are available"), Settings))
    {
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }
    const FEchoesAudioMixVolumes Restore = Settings->GetAudioMixVolumes();
    const bool bRestoreReducedRange = Settings->IsReducedDynamicRangeEnabled();

    Settings->SetMasterVolume(1.0f);
    Settings->SetReducedDynamicRangeEnabled(false);
    for (const EEchoesAudioCategory Category : EchoesAudioCategories)
    {
        Settings->SetAudioCategoryVolume(Category, 1.0f);
    }
    for (const EEchoesAudioCategory Category : EchoesAudioCategories)
    {
        Settings->SetAudioCategoryVolume(Category, 0.4f);
        Mix->ApplyPlayerVolumes();
        TestTrue(
            *FString::Printf(
                TEXT("The player's %s control reaches the graph"),
                EchoesAudioMix::CategoryStableName(Category)),
            FMath::IsNearlyEqual(
                Mix->GetAppliedCategoryGain(Category),
                0.4f,
                KINDA_SMALL_NUMBER));
        for (const EEchoesAudioCategory Other : EchoesAudioCategories)
        {
            if (Other == Category)
            {
                continue;
            }
            if (Settings->GetAudioCategoryVolume(Other) != 1.0f)
            {
                // Already lowered by an earlier iteration; only untouched
                // categories carry the unit-gain expectation.
                continue;
            }
            TestTrue(
                *FString::Printf(
                    TEXT("The %s control does not move %s"),
                    EchoesAudioMix::CategoryStableName(Category),
                    EchoesAudioMix::CategoryStableName(Other)),
                FMath::IsNearlyEqual(
                    Mix->GetAppliedCategoryGain(Other),
                    1.0f,
                    KINDA_SMALL_NUMBER));
        }
    }
    TestTrue(TEXT("Category volumes round-trip through the settings object"),
             FMath::IsNearlyEqual(Settings->GetMusicVolume(), 0.4f) &&
                 FMath::IsNearlyEqual(Settings->GetDialogueVolume(), 0.4f) &&
                 FMath::IsNearlyEqual(Settings->GetInterfaceVolume(), 0.4f) &&
                 FMath::IsNearlyEqual(Settings->GetAmbienceVolume(), 0.4f) &&
                 FMath::IsNearlyEqual(Settings->GetEffectsVolume(), 0.4f));

    // The pre-existing effects-volume contract must survive the new graph.
    Settings->SetEffectsVolume(0.5f);
    TestTrue(TEXT("The existing effects-volume control is unchanged"),
             FMath::IsNearlyEqual(Settings->GetEffectsVolume(), 0.5f) &&
                 FMath::IsNearlyEqual(
                     Settings->GetAudioCategoryVolume(
                         EEchoesAudioCategory::Effects),
                     0.5f));

    Settings->SetMasterVolume(Restore.Master);
    Settings->SetMusicVolume(Restore.Music);
    Settings->SetDialogueVolume(Restore.Dialogue);
    Settings->SetInterfaceVolume(Restore.Interface);
    Settings->SetAmbienceVolume(Restore.Ambience);
    Settings->SetEffectsVolume(Restore.Effects);
    Settings->SetReducedDynamicRangeEnabled(bRestoreReducedRange);
    Mix->ApplyPlayerVolumes();

    return true;
}

#endif

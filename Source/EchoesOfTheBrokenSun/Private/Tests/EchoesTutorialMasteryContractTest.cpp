#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "EchoesTestSaveEnvironment.h"

#include "EchoesPlayerController.h"
#include "EchoesPlayerFlow.h"
#include "EchoesPlayerProfile.h"
#include "EchoesTutorialCurriculumModel.h"
#include "EchoesSimulationSubsystem.h"
#include "Engine/World.h"
#include "HAL/FileManager.h"
#include "Misc/Crc.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Tests/AutomationCommon.h"

// A uniquely named namespace, not an anonymous one. Unreal batches several
// .cpp files into one unity translation unit, and every anonymous namespace in
// that unit is the SAME namespace: two file-local helpers sharing a signature
// become a redefinition, and a sibling's parameter shadows a constant here.
// Each file still compiles alone, so nothing catches it until the batch.
namespace EchoesTutorialMasteryContractDetail
{
// Profile.sav layout: 8-byte magic, u16 schema, u16 payload size, then the
// payload (slot u8, flags u8, lesson mask u16, ...) and a trailing u32 CRC.
constexpr int32 ProfileLessonMaskOffset = 14;
/** Last payload byte, immediately before the trailing checksum. */
constexpr int32 ProfileContractWidthTrailingOffset = 5;

void WriteMask(TArray<uint8>& Bytes, uint16 Value)
{
    Bytes[ProfileLessonMaskOffset] = static_cast<uint8>(Value);
    Bytes[ProfileLessonMaskOffset + 1] = static_cast<uint8>(Value >> 8);
}

/** Restate the contract width this record was written under. */
void WriteContractWidth(TArray<uint8>& Bytes, uint8 LessonCount)
{
    Bytes[Bytes.Num() - ProfileContractWidthTrailingOffset] = LessonCount;
}

void RefreshChecksum(TArray<uint8>& Bytes)
{
    const int32 ChecksumOffset = Bytes.Num() - 4;
    const uint32 Checksum = FCrc::MemCrc32(Bytes.GetData(), ChecksumOffset);
    for (int32 Index = 0; Index < 4; ++Index)
    {
        Bytes[ChecksumOffset + Index] =
            static_cast<uint8>(Checksum >> (Index * 8));
    }
}

/** A profile that has verified the whole implemented curriculum. */
FEchoesPlayerProfile MasteredProfile()
{
    FEchoesPlayerProfile Profile;
    Profile.ActiveJourneySlot = 1;
    Profile.bOnboardingOffered = true;
    Profile.TutorialVerifiedMask = FEchoesPlayerProfile::AllTutorialLessonsMask;
    Profile.bReadinessOperationVerified = true;
    return Profile;
}
}  // namespace EchoesTutorialMasteryContractDetail

/**
 * The completion contract. `AllTutorialLessonsMask` (the contract) and
 * `ImplementedLessonMask` (the earnable set) were independent literals, so
 * completion demanded ten verified lessons while only five could be awarded
 * and `IsTutorialMasteryComplete()` was a constant false. This asserts the
 * contract is reachable and that it is still not granted for free.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEchoesTutorialMasteryContractTest,
    "Echoes.Runtime.Campaign.TutorialMasteryContract",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::ClientContext |
        EAutomationTestFlags::EngineFilter)

bool FEchoesTutorialMasteryContractTest::RunTest(const FString& Parameters)
{
    (void)Parameters;

    // --- One source of truth ------------------------------------------------
    static_assert(
        FEchoesPlayerProfile::AllTutorialLessonsMask ==
            FEchoesTutorialPracticeState::ImplementedLessonMask,
        "the completion contract and the earnable set must be one constant");
    static_assert(
        FEchoesPlayerProfile::AllTutorialLessonsMask ==
            static_cast<uint16>((1u << EchoesTutorialLessonCount) - 1u),
        "the contract must be the implemented curriculum's contiguous prefix");
    TestEqual(
        TEXT("The completion contract equals the practice gate"),
        FEchoesPlayerProfile::AllTutorialLessonsMask,
        FEchoesTutorialPracticeState::ImplementedLessonMask);

    // --- Mastery is reachable ----------------------------------------------
    const FEchoesPlayerProfile Mastered = EchoesTutorialMasteryContractDetail::MasteredProfile();
    TestTrue(
        TEXT("Verifying every implemented lesson plus readiness completes mastery"),
        Mastered.IsTutorialMasteryComplete());

    // Every lesson short of the contract, one at a time, still withholds it.
    for (int32 Index = 0; Index < EchoesTutorialLessonCount; ++Index)
    {
        FEchoesPlayerProfile Short = Mastered;
        Short.TutorialVerifiedMask = static_cast<uint16>(
            (1u << Index) - 1u);
        TestFalse(
            FString::Printf(
                TEXT("A curriculum stopping at lesson %d does not complete mastery"),
                Index),
            Short.IsTutorialMasteryComplete());
    }

    // The readiness operation remains a separate, independent proof.
    FEchoesPlayerProfile LessonsOnly = Mastered;
    LessonsOnly.bReadinessOperationVerified = false;
    TestFalse(
        TEXT("Lessons without the readiness operation do not complete mastery"),
        LessonsOnly.IsTutorialMasteryComplete());

    FEchoesScopedTestSaveEnvironment SaveEnvironment(*this);
    if (!SaveEnvironment.IsReady()) return false;
    FString Feedback;

    // --- Readiness proof still cannot outrun the curriculum ----------------
    const FString RefusedPath = FPaths::Combine(
        SaveEnvironment.Directory, TEXT("ProfileRefused.sav"));
    FEchoesPlayerProfile Impossible = Mastered;
    Impossible.TutorialVerifiedMask = static_cast<uint16>(
        FEchoesPlayerProfile::AllTutorialLessonsMask >> 1);
    TestFalse(
        TEXT("Readiness proof without the whole curriculum is refused"),
        FEchoesPlayerProfileStore::SaveAtomic(
            RefusedPath, Impossible, Feedback));
    TestTrue(
        TEXT("The refusal names the readiness invariant"),
        Feedback.Contains(TEXT("PROFILE_READINESS_STATE_INVALID")));

    // --- Mastery survives a cold restart -----------------------------------
    const FString MasteredPath = FPaths::Combine(
        SaveEnvironment.Directory, TEXT("ProfileMastered.sav"));
    TestTrue(
        TEXT("A mastered profile commits"),
        FEchoesPlayerProfileStore::SaveAtomic(
            MasteredPath, Mastered, Feedback));
    FEchoesPlayerProfile Reloaded;
    bool bExists = false;
    TestTrue(
        TEXT("A mastered profile reloads from disk with mastery intact"),
        FEchoesPlayerProfileStore::LoadWithBackup(
            MasteredPath, Reloaded, bExists, Feedback) &&
            bExists && Reloaded == Mastered &&
            Reloaded.IsTutorialMasteryComplete());

    // --- A profile written under a different contract migrates -------------
    // The contract width the record carries is what separates a curriculum
    // that grew from a proof that was forged. Without it the store would have
    // to choose between deleting real players' profiles when lessons land and
    // accepting a readiness claim that never covered the contract.
    TArray<uint8> Bytes;
    if (!TestTrue(
            TEXT("The mastered profile bytes can be inspected"),
            FFileHelper::LoadFileToArray(Bytes, *MasteredPath)))
    {
        return false;
    }

    // Narrower than today's contract: the lessons stand, the claim does not.
    {
        TArray<uint8> Narrow = Bytes;
        const uint16 NarrowMask = static_cast<uint16>(
            FEchoesPlayerProfile::AllTutorialLessonsMask >> 1);
        EchoesTutorialMasteryContractDetail::WriteMask(Narrow, NarrowMask);
        EchoesTutorialMasteryContractDetail::WriteContractWidth(
            Narrow, static_cast<uint8>(EchoesTutorialLessonCount - 1));
        EchoesTutorialMasteryContractDetail::RefreshChecksum(Narrow);
        const FString NarrowPath = FPaths::Combine(
            SaveEnvironment.Directory, TEXT("ProfileNarrowContract.sav"));
        TestTrue(
            TEXT("A narrower-contract fixture is written"),
            FFileHelper::SaveArrayToFile(Narrow, *NarrowPath));
        FEchoesPlayerProfile Migrated;
        TestTrue(
            TEXT("A profile from a narrower contract still loads"),
            FEchoesPlayerProfileStore::LoadWithBackup(
                NarrowPath, Migrated, bExists, Feedback) && bExists);
        TestEqual(
            TEXT("Migration keeps the lessons the player genuinely verified"),
            Migrated.TutorialVerifiedMask, NarrowMask);
        TestFalse(
            TEXT("Migration withdraws a readiness claim it no longer covers"),
            Migrated.bReadinessOperationVerified);
        TestFalse(
            TEXT("Migration never fabricates mastery"),
            Migrated.IsTutorialMasteryComplete());
    }

    // Wider: the surplus bits go, and the claim stands because the player
    // verified at least everything the contract now asks for.
    {
        TArray<uint8> Wide = Bytes;
        EchoesTutorialMasteryContractDetail::WriteMask(Wide, 0x03FF);
        EchoesTutorialMasteryContractDetail::WriteContractWidth(
            Wide, static_cast<uint8>(EchoesTutorialAuthoredLessonKeys));
        EchoesTutorialMasteryContractDetail::RefreshChecksum(Wide);
        const FString WidePath = FPaths::Combine(
            SaveEnvironment.Directory, TEXT("ProfileWideContract.sav"));
        TestTrue(
            TEXT("A wider-contract fixture is written"),
            FFileHelper::SaveArrayToFile(Wide, *WidePath));
        FEchoesPlayerProfile Migrated;
        TestTrue(
            TEXT("A profile from a wider contract still loads"),
            FEchoesPlayerProfileStore::LoadWithBackup(
                WidePath, Migrated, bExists, Feedback) && bExists);
        TestEqual(
            TEXT("Migration clamps a wider mask to the current contract"),
            Migrated.TutorialVerifiedMask,
            FEchoesPlayerProfile::AllTutorialLessonsMask);
        TestTrue(
            TEXT("A claim covering the whole current contract survives"),
            Migrated.IsTutorialMasteryComplete());
    }

    // Same width, incomplete mask: that is a forged claim, and it stays refused.
    {
        TArray<uint8> Forged = Bytes;
        EchoesTutorialMasteryContractDetail::WriteMask(
            Forged,
            static_cast<uint16>(
                FEchoesPlayerProfile::AllTutorialLessonsMask >> 1));
        EchoesTutorialMasteryContractDetail::RefreshChecksum(Forged);
        const FString ForgedPath = FPaths::Combine(
            SaveEnvironment.Directory, TEXT("ProfileForgedReadiness.sav"));
        TestTrue(
            TEXT("A forged-readiness fixture is written"),
            FFileHelper::SaveArrayToFile(Forged, *ForgedPath));
        FEchoesPlayerProfile Rejected;
        TestFalse(
            TEXT("Readiness under an unchanged contract is not migrated away"),
            FEchoesPlayerProfileStore::LoadWithBackup(
                ForgedPath, Rejected, bExists, Feedback));
        TestTrue(
            TEXT("The refusal names the readiness invariant"),
            Feedback.Contains(TEXT("PROFILE_READINESS_STATE_INVALID")));
    }

    // --- The title screen stops leading with the tutorial ------------------
    FTestWorldWrapper Wrapper;
    if (!Wrapper.CreateTestWorld(EWorldType::Game))
    {
        Wrapper.ForwardErrorMessages(this);
        AddError(TEXT("Could not create the mastery-contract test world."));
        return false;
    }
    UWorld* World = Wrapper.GetTestWorld();
    auto* Bridge = World != nullptr
        ? World->GetSubsystem<UEchoesSimulationSubsystem>() : nullptr;
    // PresentTitleScreen refuses without a staged scenario, so the title order
    // can only be read over one.
    if (!TestTrue(
            TEXT("A scenario stages for the title-order check"),
            Bridge != nullptr && Bridge->StartPrototypeScenario()))
    {
        Wrapper.ForwardErrorMessages(this);
        return false;
    }
    // The title screen is a paused state, and journey-slot selection -- which
    // profile initialization performs -- refuses to run over a live scenario.
    Bridge->SetScenarioPaused(true);

    const auto FirstButtonAction = [](const FEchoesShellView& View)
    {
        return View.Buttons.IsEmpty()
            ? EEchoesShellAction::Back : View.Buttons[0].Action;
    };
    // A controller reads the profile once, so each profile needs its own.
    const auto TitleViewFor =
        [&](const FEchoesPlayerProfile& Profile, bool bExpectMastery)
    {
        FString SaveFeedback;
        TestTrue(
            TEXT("The title-order profile persists"),
            FEchoesPlayerProfileStore::SaveAtomic(
                FEchoesPlayerProfileStore::GetDefaultPath(),
                Profile, SaveFeedback));
        auto* TitleController = World->SpawnActor<AEchoesPlayerController>();
        FEchoesShellView View;
        if (TitleController == nullptr)
        {
            AddError(TEXT("The title-order controller did not spawn."));
            return View;
        }
        TitleController->InitInputSystem();
        TestTrue(
            TEXT("The title-order profile initializes"),
            TitleController->InitializePlayerProfile());
        TestEqual(
            TEXT("The title-order profile carries the expected mastery"),
            TitleController->GetPlayerProfile().IsTutorialMasteryComplete(),
            bExpectMastery);
        TitleController->PresentTitleScreen();
        View = TitleController->BuildShellView();
        TestTrue(
            TEXT("The title screen is the presented screen"),
            View.Screen == EEchoesShellScreen::Title);
        TitleController->Destroy();
        return View;
    };

    TestTrue(
        TEXT("Before mastery the title screen offers the tutorial first"),
        FirstButtonAction(TitleViewFor(FEchoesPlayerProfile{}, false)) ==
            EEchoesShellAction::Tutorial);

    const FEchoesShellView MasteredTitle = TitleViewFor(Mastered, true);
    TestTrue(
        TEXT("Once mastery is held the title screen leads with the campaign"),
        FirstButtonAction(MasteredTitle) == EEchoesShellAction::Campaign);
    bool bTutorialStillOffered = false;
    for (const auto& Button : MasteredTitle.Buttons)
    {
        bTutorialStillOffered = bTutorialStillOffered ||
            Button.Action == EEchoesShellAction::Tutorial;
    }
    TestTrue(
        TEXT("The tutorial stays replayable after mastery"),
        bTutorialStillOffered);

    Bridge->StopPrototypeScenario();
    Wrapper.ForwardErrorMessages(this);
    return true;
}

#endif

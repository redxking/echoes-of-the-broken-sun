#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "EchoesTutorialCurriculumModel.h"

namespace
{
/** Facts for a lesson the player has completed against authoritative state. */
FEchoesTutorialLessonFacts VerifiedFacts()
{
    FEchoesTutorialLessonFacts Facts;
    Facts.bCurriculumActive = true;
    Facts.bLessonOpened = true;
    Facts.bActionObserved = true;
    Facts.bAuthoritativeStateVerified = true;
    Facts.LessonOrdinal = 0;
    return Facts;
}

/** Facts for a lesson whose instruction has been delivered and nothing more. */
FEchoesTutorialLessonFacts OpenedFacts()
{
    FEchoesTutorialLessonFacts Facts;
    Facts.bCurriculumActive = true;
    Facts.bLessonOpened = true;
    Facts.LessonOrdinal = 0;
    return Facts;
}

TArray<FEchoesTutorialLessonFacts> CurriculumVerifiedThrough(int32 VerifiedCount)
{
    TArray<FEchoesTutorialLessonFacts> Facts;
    Facts.Reserve(EchoesTutorialLessonCount);
    for (int32 Index = 0; Index < EchoesTutorialLessonCount; ++Index)
    {
        FEchoesTutorialLessonFacts Lesson =
            Index < VerifiedCount ? VerifiedFacts() : OpenedFacts();
        Lesson.LessonOrdinal = Index;
        Facts.Add(Lesson);
    }
    return Facts;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEchoesTutorialCurriculumTest,
    "Echoes.Runtime.Campaign.TutorialCurriculum",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::ClientContext |
        EAutomationTestFlags::EngineFilter)

bool FEchoesTutorialCurriculumTest::RunTest(const FString& Parameters)
{
    (void)Parameters;

    using FModel = FEchoesTutorialCurriculumModel;

    // --- Lesson reducer -------------------------------------------------
    FEchoesTutorialLessonFacts Facts;
    Facts.LessonOrdinal = 0;
    TestTrue(TEXT("An inactive curriculum locks every lesson"),
             FModel::DetermineLessonState(Facts, true, 0) ==
                 EEchoesTutorialLessonState::Locked);

    Facts.bCurriculumActive = true;
    TestTrue(TEXT("An active lesson with no instruction stays locked"),
             FModel::DetermineLessonState(Facts, true, 0) ==
                 EEchoesTutorialLessonState::Locked);

    Facts.bLessonOpened = true;
    TestTrue(TEXT("Delivered instruction opens the lesson but does not complete it"),
             FModel::DetermineLessonState(Facts, true, 0) ==
                 EEchoesTutorialLessonState::Open);

    Facts.bActionObserved = true;
    TestTrue(TEXT("An observed action alone does not complete the lesson"),
             FModel::DetermineLessonState(Facts, true, 0) ==
                 EEchoesTutorialLessonState::Acting);

    FEchoesTutorialLessonFacts Mistaken = OpenedFacts();
    Mistaken.bRecoverableFault = true;
    TestTrue(TEXT("A recoverable mistake keeps the lesson open for another attempt"),
             FModel::DetermineLessonState(Mistaken, true, 0) ==
                 EEchoesTutorialLessonState::Acting);

    Facts.bAuthoritativeStateVerified = true;
    TestTrue(TEXT("Only authoritative state completes a lesson"),
             FModel::DetermineLessonState(Facts, true, 0) ==
                 EEchoesTutorialLessonState::Verified);

    TestTrue(TEXT("An unmet prerequisite locks a lesson even when its outcome is observed"),
             FModel::DetermineLessonState(Facts, false, 0) ==
                 EEchoesTutorialLessonState::Locked);

    FEchoesTutorialLessonFacts Lost = VerifiedFacts();
    Lost.bUnrecoverableFault = true;
    TestTrue(TEXT("An unrecoverable fault fails a lesson the player has reached"),
             FModel::DetermineLessonState(Lost, true, 0) ==
                 EEchoesTutorialLessonState::Failed);
    TestTrue(TEXT("An unrecoverable fault on an UNREACHED lesson locks it rather than failing"),
             FModel::DetermineLessonState(Lost, false, 0) ==
                 EEchoesTutorialLessonState::Locked);

    // --- Curriculum reducer ---------------------------------------------
    const FEchoesTutorialCurriculumState Fresh =
        FModel::DetermineCurriculumState(CurriculumVerifiedThrough(0));
    TestTrue(TEXT("A fresh curriculum opens its first lesson and locks the rest"),
             Fresh.Lessons[0] == EEchoesTutorialLessonState::Open &&
                 Fresh.Lessons[1] == EEchoesTutorialLessonState::Locked &&
                 Fresh.ActiveLessonIndex == 0 && !Fresh.bMasteryComplete);

    const FEchoesTutorialCurriculumState Midway =
        FModel::DetermineCurriculumState(CurriculumVerifiedThrough(3));
    TestTrue(TEXT("Verified lessons unlock exactly the next lesson in contract order"),
             Midway.Lessons[2] == EEchoesTutorialLessonState::Verified &&
                 Midway.Lessons[3] == EEchoesTutorialLessonState::Open &&
                 Midway.Lessons[4] == EEchoesTutorialLessonState::Locked &&
                 Midway.ActiveLessonIndex == 3 && !Midway.bMasteryComplete);

    const FEchoesTutorialCurriculumState Mastered =
        FModel::DetermineCurriculumState(
            CurriculumVerifiedThrough(EchoesTutorialLessonCount));
    TestTrue(TEXT("Every verified lesson completes mastery and leaves no active lesson"),
             Mastered.bMasteryComplete && !Mastered.bFailed &&
                 Mastered.ActiveLessonIndex == INDEX_NONE);

    // A later lesson finished early must not skip the chain or grant mastery.
    TArray<FEchoesTutorialLessonFacts> OutOfOrder = CurriculumVerifiedThrough(0);
    OutOfOrder[EchoesTutorialLessonCount - 1] = VerifiedFacts();
    const FEchoesTutorialCurriculumState Skipped =
        FModel::DetermineCurriculumState(OutOfOrder);
    TestTrue(TEXT("An out-of-order outcome cannot skip the chain or grant mastery"),
             Skipped.Lessons[EchoesTutorialLessonCount - 1] ==
                     EEchoesTutorialLessonState::Locked &&
                 !Skipped.bMasteryComplete && Skipped.ActiveLessonIndex == 0);

    TArray<FEchoesTutorialLessonFacts> WithLoss =
        CurriculumVerifiedThrough(EchoesTutorialLessonCount);
    WithLoss[5].bUnrecoverableFault = true;
    const FEchoesTutorialCurriculumState Failed =
        FModel::DetermineCurriculumState(WithLoss);
    TestTrue(TEXT("An unrecoverable fault fails the curriculum and withholds mastery"),
             Failed.bFailed && !Failed.bMasteryComplete &&
                 Failed.ActiveLessonIndex == 5);

    const FEchoesTutorialCurriculumState Malformed =
        FModel::DetermineCurriculumState(
            TArrayView<const FEchoesTutorialLessonFacts>());
    TestTrue(TEXT("A malformed curriculum fails closed and unlocks nothing"),
             !Malformed.bMasteryComplete &&
                 Malformed.ActiveLessonIndex == INDEX_NONE &&
                 Malformed.Lessons[0] == EEchoesTutorialLessonState::Locked);

    // --- Caller-consistency (synthesis; gaps closed from 6db209b) --------
    // These describe sequences that cannot occur in real play, so each must
    // withhold progress WITHOUT declaring the player failed.
    FEchoesTutorialLessonFacts NeverPresented = VerifiedFacts();
    NeverPresented.bLessonOpened = false;
    TestTrue(TEXT("A lesson verified without ever being presented is refused"),
             FModel::AreLessonFactsMalformed(NeverPresented, 0) &&
                 FModel::DetermineLessonState(NeverPresented, true, 0) ==
                     EEchoesTutorialLessonState::Locked);

    FEchoesTutorialLessonFacts ActedUnprompted = OpenedFacts();
    ActedUnprompted.bLessonOpened = false;
    ActedUnprompted.bActionObserved = true;
    TestTrue(TEXT("An action on a lesson never presented is refused"),
             FModel::AreLessonFactsMalformed(ActedUnprompted, 0) &&
                 FModel::DetermineLessonState(ActedUnprompted, true, 0) ==
                     EEchoesTutorialLessonState::Locked);

    FEchoesTutorialLessonFacts VerifiedWithoutAction = VerifiedFacts();
    VerifiedWithoutAction.bActionObserved = false;
    TestTrue(TEXT("Verification without an observed action is refused"),
             FModel::AreLessonFactsMalformed(VerifiedWithoutAction, 0));

    FEchoesTutorialLessonFacts Conflicting = VerifiedFacts();
    Conflicting.bUnrecoverableFault = true;
    TestTrue(TEXT("Conflicting terminal facts lock rather than fail the lesson"),
             FModel::AreLessonFactsMalformed(Conflicting, 0) &&
                 FModel::DetermineLessonState(Conflicting, true, 0) ==
                     EEchoesTutorialLessonState::Locked);

    TArray<FEchoesTutorialLessonFacts> Shuffled =
        CurriculumVerifiedThrough(EchoesTutorialLessonCount);
    Shuffled[4].LessonOrdinal = 7;
    const FEchoesTutorialCurriculumState Misaligned =
        FModel::DetermineCurriculumState(Shuffled);
    TestTrue(TEXT("An ordinal disagreeing with its position withholds mastery without failing"),
             Misaligned.bMalformed && !Misaligned.bMasteryComplete &&
                 !Misaligned.bFailed);

    TestTrue(TEXT("A well-formed mastered curriculum is not flagged malformed"),
             !FModel::DetermineCurriculumState(
                  CurriculumVerifiedThrough(EchoesTutorialLessonCount))
                  .bMalformed);

    // --- Review findings F10-F13 ----------------------------------------
    // F10: a fault reported on a lesson the player has not reached must not
    // fail the curriculum. The reducer distrusts a caller's terminal-failure
    // fact about an unreached lesson exactly as it distrusts terminal success.
    TArray<FEchoesTutorialLessonFacts> FaultAhead = CurriculumVerifiedThrough(0);
    FaultAhead[EchoesTutorialLessonCount - 1].bUnrecoverableFault = true;
    const FEchoesTutorialCurriculumState Unreached =
        FModel::DetermineCurriculumState(FaultAhead);
    TestTrue(TEXT("A fault on an unreached lesson does not fail the curriculum"),
             !Unreached.bFailed && Unreached.ActiveLessonIndex == 0 &&
                 Unreached.Lessons[EchoesTutorialLessonCount - 1] ==
                     EEchoesTutorialLessonState::Locked);

    // F11: a recoverable fault is player behaviour and cannot occur on a
    // lesson that was never presented; it must not reach Acting.
    FEchoesTutorialLessonFacts FaultUnprompted;
    FaultUnprompted.LessonOrdinal = 0;
    FaultUnprompted.bCurriculumActive = true;
    FaultUnprompted.bRecoverableFault = true;
    TestTrue(TEXT("A recoverable fault on an unpresented lesson is refused, not Acting"),
             FModel::AreLessonFactsMalformed(FaultUnprompted, 0) &&
                 FModel::DetermineLessonState(FaultUnprompted, true, 0) ==
                     EEchoesTutorialLessonState::Locked);

    // F12: activity reported on a lesson whose prerequisite is unmet means the
    // fact-deriver is broken, even though the ordering gate keeps it safe.
    TArray<FEchoesTutorialLessonFacts> ActivityAhead = CurriculumVerifiedThrough(0);
    for (int32 Index = 5; Index < EchoesTutorialLessonCount; ++Index)
    {
        ActivityAhead[Index] = VerifiedFacts();
        ActivityAhead[Index].LessonOrdinal = Index;
    }
    const FEchoesTutorialCurriculumState Impossible =
        FModel::DetermineCurriculumState(ActivityAhead);
    TestTrue(TEXT("Activity on unreachable lessons is reported as malformed"),
             Impossible.bMalformed && !Impossible.bMasteryComplete &&
                 Impossible.ActiveLessonIndex == 0);

    // F13: the position check must use the caller's expected ordinal, so a
    // wrong ordinal cannot pass by being compared against itself.
    FEchoesTutorialLessonFacts WrongPosition = VerifiedFacts();
    WrongPosition.LessonOrdinal = 999;
    TestTrue(TEXT("A wrong ordinal is refused through the lesson reducer itself"),
             FModel::DetermineLessonState(WrongPosition, true, 0) ==
                 EEchoesTutorialLessonState::Locked);

    // --- Data/code correspondence ---------------------------------------
    // These keys are the shipped demo contract's lesson keys in its trigger
    // prerequisite order (Content/Narrative/Source/demo/tutorial_readiness_check.json).
    const TCHAR* const ExpectedKeys[EchoesTutorialLessonCount] = {
        TEXT("survey"), TEXT("roster"), TEXT("muster"), TEXT("route"),
        TEXT("reserve"), TEXT("link"), TEXT("foundry"), TEXT("probe"),
        TEXT("board"), TEXT("well")};
    bool bKeysMatch = true;
    for (int32 Index = 0; Index < EchoesTutorialLessonCount; ++Index)
    {
        bKeysMatch = bKeysMatch &&
            FString(FModel::StableName(
                static_cast<EEchoesTutorialLesson>(Index))) ==
                FString(ExpectedKeys[Index]);
    }
    TestTrue(TEXT("Lesson order and keys match the shipped demo narrative contract"),
             bKeysMatch);

    return true;
}

#endif

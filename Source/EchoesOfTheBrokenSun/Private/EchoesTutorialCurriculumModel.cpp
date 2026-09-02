#include "EchoesTutorialCurriculumModel.h"

bool FEchoesTutorialCurriculumModel::AreLessonFactsMalformed(
    const FEchoesTutorialLessonFacts& Facts,
    int32 ExpectedOrdinal)
{
    // Caller-consistency checks. These describe sequences that cannot happen
    // in real play, so they indicate a defect in whatever derived the facts —
    // never a mistake by the player.
    if (Facts.LessonOrdinal != ExpectedOrdinal)
    {
        // A shuffled or misaligned curriculum array.
        return true;
    }
    if (Facts.bActionObserved && !Facts.bLessonOpened)
    {
        // The player cannot act on a lesson that was never presented.
        return true;
    }
    if (Facts.bRecoverableFault && !Facts.bLessonOpened)
    {
        // A recoverable fault records player behaviour just as an observed
        // action does, so it is equally impossible on an unpresented lesson.
        // Without this, an unpresented lesson could reach Acting — the exact
        // state hesitation escalation keys off.
        return true;
    }
    if (Facts.bAuthoritativeStateVerified && !Facts.bActionObserved)
    {
        // Verification without an observed action would let a caller mark a
        // lesson complete that the player never performed.
        return true;
    }
    if (Facts.bAuthoritativeStateVerified && Facts.bUnrecoverableFault)
    {
        // Conflicting terminal facts: neither can be trusted.
        return true;
    }
    return false;
}

EEchoesTutorialLessonState FEchoesTutorialCurriculumModel::DetermineLessonState(
    const FEchoesTutorialLessonFacts& Facts,
    bool bPrerequisiteVerified,
    int32 ExpectedOrdinal)
{
    if (AreLessonFactsMalformed(Facts, ExpectedOrdinal))
    {
        // Withhold progress without declaring player failure. The curriculum
        // reducer additionally validates the ordinal against real position.
        return EEchoesTutorialLessonState::Locked;
    }
    if (!Facts.bCurriculumActive)
    {
        return EEchoesTutorialLessonState::Locked;
    }
    if (!bPrerequisiteVerified)
    {
        // Ordering is enforced here rather than trusted from the caller, so a
        // later lesson cannot be reached — or verified — out of sequence even
        // if its own outcome happens to be observed early. This gate precedes
        // the fault check deliberately: the reducer must not trust a caller's
        // terminal-FAILURE fact about an unreached lesson any more than it
        // trusts a terminal-SUCCESS one. A fault reported on a lesson the
        // player has not reached locks it; it cannot fail the curriculum.
        return EEchoesTutorialLessonState::Locked;
    }
    if (Facts.bUnrecoverableFault)
    {
        return EEchoesTutorialLessonState::Failed;
    }
    if (Facts.bAuthoritativeStateVerified)
    {
        return EEchoesTutorialLessonState::Verified;
    }
    if (Facts.bActionObserved || Facts.bRecoverableFault)
    {
        // A mistaken action keeps the lesson open for another attempt: an
        // incorrect action must produce feedback, not punishment.
        return EEchoesTutorialLessonState::Acting;
    }
    if (Facts.bLessonOpened)
    {
        // Instruction delivered is not learning: this stops at Open.
        return EEchoesTutorialLessonState::Open;
    }
    return EEchoesTutorialLessonState::Locked;
}

FEchoesTutorialCurriculumState
FEchoesTutorialCurriculumModel::DetermineCurriculumState(
    TArrayView<const FEchoesTutorialLessonFacts> Facts)
{
    FEchoesTutorialCurriculumState State;
    if (Facts.Num() != EchoesTutorialLessonCount)
    {
        // Fail closed: a malformed curriculum teaches nothing and unlocks
        // nothing. Every lesson stays Locked and mastery stays false.
        for (int32 Index = 0; Index < EchoesTutorialLessonCount; ++Index)
        {
            State.Lessons[Index] = EEchoesTutorialLessonState::Locked;
        }
        State.ActiveLessonIndex = INDEX_NONE;
        State.bMalformed = true;
        return State;
    }

    bool bPrerequisiteVerified = true;
    bool bMasteryComplete = true;
    for (int32 Index = 0; Index < EchoesTutorialLessonCount; ++Index)
    {
        const bool bActivityReported =
            Facts[Index].bLessonOpened || Facts[Index].bActionObserved ||
            Facts[Index].bAuthoritativeStateVerified ||
            Facts[Index].bRecoverableFault;
        if (AreLessonFactsMalformed(Facts[Index], Index) ||
            (!bPrerequisiteVerified && bActivityReported))
        {
            // The second case: a fact-deriver reporting activity on a lesson
            // the player cannot have reached. The ordering gate already keeps
            // the outcome safe, but silence would let that defect persist.
            State.bMalformed = true;
        }
        const EEchoesTutorialLessonState LessonState =
            AreLessonFactsMalformed(Facts[Index], Index)
                ? EEchoesTutorialLessonState::Locked
                : DetermineLessonState(
                      Facts[Index], bPrerequisiteVerified, Index);
        State.Lessons[Index] = LessonState;
        if (LessonState == EEchoesTutorialLessonState::Failed)
        {
            State.bFailed = true;
        }
        if (LessonState != EEchoesTutorialLessonState::Verified)
        {
            bMasteryComplete = false;
            if (State.ActiveLessonIndex == INDEX_NONE)
            {
                State.ActiveLessonIndex = Index;
            }
        }
        bPrerequisiteVerified =
            bPrerequisiteVerified &&
            LessonState == EEchoesTutorialLessonState::Verified;
    }
    State.bMasteryComplete =
        bMasteryComplete && !State.bFailed && !State.bMalformed;
    return State;
}

const TCHAR* FEchoesTutorialCurriculumModel::StableName(
    EEchoesTutorialLesson Lesson)
{
    switch (Lesson)
    {
        case EEchoesTutorialLesson::Survey: return TEXT("survey");
        case EEchoesTutorialLesson::Roster: return TEXT("roster");
        case EEchoesTutorialLesson::Muster: return TEXT("muster");
        case EEchoesTutorialLesson::Route: return TEXT("route");
        case EEchoesTutorialLesson::Reserve: return TEXT("reserve");
        case EEchoesTutorialLesson::Link: return TEXT("link");
        case EEchoesTutorialLesson::Foundry: return TEXT("foundry");
        case EEchoesTutorialLesson::Probe: return TEXT("probe");
        case EEchoesTutorialLesson::Board: return TEXT("board");
        case EEchoesTutorialLesson::Well: return TEXT("well");
    }
    return TEXT("unknown");
}

#pragma once

#include "CoreMinimal.h"

/**
 * Pure reducers for the demo tutorial curriculum ("The Readiness Check").
 *
 * The lesson cycle is binding: explain -> highlight -> allow the player to act
 * -> VERIFY THE REAL GAME STATE -> acknowledge -> explain why -> unlock the
 * next lesson. A displayed prompt, a dismissed message, a timer, or a trigger
 * volume never advances a lesson: only `bAuthoritativeStateVerified`, which the
 * caller may set solely from authoritative simulation or controller state, can
 * reach `Verified`.
 *
 * Lesson order here is the same order the shipped demo narrative contract
 * encodes in its trigger prerequisite chain
 * (`Content/Narrative/Source/demo/tutorial_readiness_check.json`), so the data
 * and the code agree by construction; `StableName` returns that contract's
 * lesson keys verbatim.
 *
 * These reducers are presentation- and simulation-independent: they own no
 * state, read no globals, and never enter simulation state, saves, replays, or
 * checksums.
 */

enum class EEchoesTutorialLesson : uint8
{
    Survey = 0,
    Roster,
    Muster,
    Route,
    Reserve,
    Link,
    Foundry,
    Probe,
    Board,
    Well,
};

inline constexpr int32 EchoesTutorialLessonCount = 10;

enum class EEchoesTutorialLessonState : uint8
{
    /** Not reachable yet: the curriculum is inactive or a prerequisite is unmet. */
    Locked,
    /** Instruction delivered; the player has not acted. */
    Open,
    /** The player has acted, or has acted incorrectly and may retry. */
    Acting,
    /** Authoritative state proved the required outcome. */
    Verified,
    /** An unrecoverable fault (for example a required actor was lost). */
    Failed,
};

/**
 * One lesson's observations. Every field is an observation of something that
 * already happened; none of them is a timer or a UI acknowledgement.
 */
struct FEchoesTutorialLessonFacts final
{
    /** This lesson's position in the authored order; must equal its index. */
    int32 LessonOrdinal = INDEX_NONE;
    /** The curriculum is running (mission active, force present). */
    bool bCurriculumActive = false;
    /** Instruction for this lesson has been delivered. */
    bool bLessonOpened = false;
    /** The player performed the lesson's action, correctly or not. */
    bool bActionObserved = false;
    /** Authoritative state proves the required outcome. The only path to Verified. */
    bool bAuthoritativeStateVerified = false;
    /** An invalid or mistaken action occurred; the player may retry without penalty. */
    bool bRecoverableFault = false;
    /** The lesson can no longer be completed and needs a reset. */
    bool bUnrecoverableFault = false;
};

/** The whole curriculum's derived state, including the mastery gate. */
struct FEchoesTutorialCurriculumState final
{
    /**
     * A caller supplied facts that cannot describe a real play sequence — for
     * example a lesson verified without ever being presented, or an ordinal
     * disagreeing with its position. Every unlock is withheld, but the player
     * is NOT declared to have failed: a caller defect must not soft-lock a
     * tutorial, so this is deliberately distinct from bFailed.
     */
    bool bMalformed = false;
    EEchoesTutorialLessonState Lessons[EchoesTutorialLessonCount] = {};
    /** First lesson not yet Verified, or INDEX_NONE once every lesson is Verified. */
    int32 ActiveLessonIndex = INDEX_NONE;
    /** Every lesson Verified: the DEMO-JRN-003 gate for unlocking the AI demo. */
    bool bMasteryComplete = false;
    /** Any lesson Failed. */
    bool bFailed = false;
};

struct ECHOESOFTHEBROKENSUN_API FEchoesTutorialCurriculumModel final
{
    /**
     * One lesson's state. `bPrerequisiteVerified` and `ExpectedOrdinal` are
     * supplied by the curriculum reducer, never by a caller, so neither
     * ordering nor position can be bypassed. Passing `Facts.LessonOrdinal` as
     * `ExpectedOrdinal` would make the position check vacuous; callers outside
     * the curriculum reducer must pass the position they actually expect.
     */
    [[nodiscard]] static EEchoesTutorialLessonState DetermineLessonState(
        const FEchoesTutorialLessonFacts& Facts,
        bool bPrerequisiteVerified,
        int32 ExpectedOrdinal);

    /** The ordered curriculum, enforcing the contract's prerequisite chain. */
    [[nodiscard]] static FEchoesTutorialCurriculumState DetermineCurriculumState(
        TArrayView<const FEchoesTutorialLessonFacts> Facts);

    /**
     * True when these facts cannot describe a real play sequence. Malformed
     * facts withhold progress rather than failing the player.
     */
    [[nodiscard]] static bool AreLessonFactsMalformed(
        const FEchoesTutorialLessonFacts& Facts,
        int32 ExpectedOrdinal);

    /** The demo contract's lesson key, verbatim. */
    [[nodiscard]] static const TCHAR* StableName(EEchoesTutorialLesson Lesson);
};

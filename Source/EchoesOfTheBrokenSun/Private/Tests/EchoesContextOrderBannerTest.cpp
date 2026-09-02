#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "EchoesContextOrderReport.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEchoesContextOrderBannerTest,
    "Echoes.Runtime.Controls.ContextOrderBanner",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::ClientContext |
        EAutomationTestFlags::EngineFilter)

bool FEchoesContextOrderBannerTest::RunTest(const FString& Parameters)
{
    (void)Parameters;

    AddInfo(
        TEXT("Scope: the banner text a context order produces from counted "
             "observations. SIM-003 requires the authority's own reason code "
             "and recovery text; the 'Recoverable command' pillar requires "
             "that a context order never becomes a plausible wrong claim."));

    // The defect this test exists for. Every carrying worker's Deliver is
    // refused by the authority; one non-carrying unit's substituted Move is
    // accepted. The banner may report the move and quote the refusal - it may
    // NOT announce that no selected worker was carrying Matter, because the
    // refused workers were never examined for cargo at all.
    {
        FEchoesContextOrderOutcome Outcome;
        Outcome.MovedNotCarryingCount = 1;
        Outcome.RecordRejection(41, TEXT("[DELIVER_BLOCKED] Route to the drop-off is severed."));
        Outcome.RecordRejection(42, TEXT("[DELIVER_BLOCKED] Route to the drop-off is severed."));
        const FString Banner =
            FEchoesContextOrderReport::ComposeDeliverBanner(Outcome);
        TestFalse(
            TEXT("A partly rejected Deliver never claims nothing was carried"),
            Banner.Contains(TEXT("no selected worker")));
        TestFalse(
            TEXT("A partly rejected Deliver never claims nothing was delivered"),
            Banner.Contains(TEXT("nothing was delivered")));
        TestTrue(
            TEXT("A partly rejected Deliver quotes the authority's reason"),
            Banner.Contains(TEXT("[DELIVER_BLOCKED] Route to the drop-off is severed.")));
        TestTrue(
            TEXT("A partly rejected Deliver counts the refusals"),
            Banner.Contains(TEXT("2 rejected")));
        TestTrue(
            TEXT("A partly rejected Deliver names the move it did make"),
            Banner.Contains(TEXT("1 queued with an empty hold")));
    }

    // A failed state lookup is not a cargo fact. The caller never substitutes
    // for one, so it can only reach the banner through the authority's answer.
    // Distinct reasons must resolve by the lowest stable identifier, not by
    // selection order.
    {
        FEchoesContextOrderOutcome Ascending;
        Ascending.DeliveredCount = 1;
        Ascending.RecordRejection(7, TEXT("[SEVEN] Reason for seven."));
        Ascending.RecordRejection(19, TEXT("[NINETEEN] Reason for nineteen."));

        FEchoesContextOrderOutcome Descending;
        Descending.DeliveredCount = 1;
        Descending.RecordRejection(19, TEXT("[NINETEEN] Reason for nineteen."));
        Descending.RecordRejection(7, TEXT("[SEVEN] Reason for seven."));

        TestEqual(
            TEXT("Selection order cannot change which refusal is shown"),
            FEchoesContextOrderReport::ComposeDeliverBanner(Ascending),
            FEchoesContextOrderReport::ComposeDeliverBanner(Descending));
        TestTrue(
            TEXT("The lowest refused identifier supplies the reason"),
            FEchoesContextOrderReport::ComposeDeliverBanner(Descending)
                .Contains(TEXT("[SEVEN] Reason for seven.")));
        TestEqual(
            TEXT("The recorded rejection identifier is the lowest one"),
            static_cast<int32>(Descending.RejectionEntityId),
            7);
        TestEqual(
            TEXT("Both refusals are still counted"),
            Descending.RejectedCount,
            2);
    }

    // The two substitution facts stay separate. A worker with an empty hold is
    // not the same statement as a unit that cannot carry Matter at all.
    {
        FEchoesContextOrderOutcome Mixed;
        Mixed.MovedNotCarryingCount = 2;
        Mixed.MovedCannotCarryCount = 3;
        const FString Banner =
            FEchoesContextOrderReport::ComposeDeliverBanner(Mixed);
        TestTrue(
            TEXT("A mixed substitution reports the empty holds"),
            Banner.Contains(TEXT("2 with an empty hold")));
        TestTrue(
            TEXT("A mixed substitution reports the units that cannot carry"),
            Banner.Contains(TEXT("3 unable to carry Matter")));
        TestTrue(
            TEXT("A move-only Deliver is titled as a move"),
            Banner.StartsWith(TEXT("MOVE TO DROP-OFF:")));
        TestTrue(
            TEXT("An unrejected banner ends in a bare full stop"),
            Banner.EndsWith(TEXT(".")));
        TestFalse(
            TEXT("An unrejected banner reports no refusals"),
            Banner.Contains(TEXT("rejected")));
    }

    // A clean delivery says only that.
    {
        FEchoesContextOrderOutcome Clean;
        Clean.DeliveredCount = 3;
        TestEqual(
            TEXT("A clean Deliver reports only the deliveries"),
            FEchoesContextOrderReport::ComposeDeliverBanner(Clean),
            FString(TEXT("DELIVER MATTER: 3 queued.")));
        TestEqual(
            TEXT("A clean Deliver counts three accepted commands"),
            Clean.AcceptedCount(),
            3);
    }

    // A delivery beside substituted moves reports both, without inventing a
    // cause for the moves it did not observe.
    {
        FEchoesContextOrderOutcome Split;
        Split.DeliveredCount = 2;
        Split.MovedNotCarryingCount = 1;
        TestEqual(
            TEXT("A split Deliver reports the deliveries and the moves"),
            FEchoesContextOrderReport::ComposeDeliverBanner(Split),
            FString(TEXT("DELIVER MATTER: 2 queued, 1 moved to the drop-off with an empty hold.")));
    }

    // Every other order keeps its label, its accepted count and the same
    // deterministic quoted refusal.
    {
        FEchoesContextOrderOutcome Move;
        Move.AcceptedOtherCount = 4;
        Move.RecordRejection(3, TEXT("[NO_PATH] No route to that position."));
        TestEqual(
            TEXT("A non-Deliver order quotes the authority the same way"),
            FEchoesContextOrderReport::ComposeOrderBanner(
                TEXT("MOVE / BOX"), Move),
            FString(TEXT("MOVE / BOX: 4 queued, 1 rejected. [NO_PATH] No route to that position.")));
    }

    // An authority that returned no text is reported as no text, never as an
    // invented cause.
    {
        FEchoesContextOrderOutcome Silent;
        Silent.AcceptedOtherCount = 1;
        Silent.RecordRejection(5, FString());
        TestEqual(
            TEXT("A silent refusal is counted without inventing a reason"),
            FEchoesContextOrderReport::ComposeOrderBanner(
                TEXT("ATTACK"), Silent),
            FString(TEXT("ATTACK: 1 queued, 1 rejected.")));
    }

    return true;
}

#endif

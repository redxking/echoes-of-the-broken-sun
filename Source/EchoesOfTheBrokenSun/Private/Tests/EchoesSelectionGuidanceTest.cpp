// Copyright Echoes of the Broken Sun. All Rights Reserved.
// Author: Angelis Pseftis

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "EchoesFieldHudView.h"
#include "EchoesSimCore/Simulation.h"

#include <array>

namespace
{
using echoes::sim::EntityType;
using echoes::sim::Faction;

/** The eight roster slots each faction fills. */
constexpr std::array<EntityType, 8> GuidanceRosterTypes = {
    EntityType::Worker,
    EntityType::Soldier,
    EntityType::HeavyUnit,
    EntityType::ScoutUnit,
    EntityType::CommandCore,
    EntityType::Dropoff,
    EntityType::Barracks,
    EntityType::UtilityStructure,
};

constexpr std::array<Faction, 3> GuidanceFactions = {
    Faction::MeridianCompact,
    Faction::KharuunAssemblies,
    Faction::HollowChoir,
};

/** World objects belong to no faction roster and must carry no authored guidance. */
constexpr std::array<EntityType, 2> GuidanceNeutralTypes = {
    EntityType::ResourceNode,
    EntityType::FutureWell,
};

FString SlotName(const Faction FactionValue, const EntityType Type)
{
    return FString::Printf(TEXT("faction %d, type %d"),
        static_cast<int32>(FactionValue), static_cast<int32>(Type));
}
}

/**
 * SPEC-HUD-003 requires every selection to answer purpose, strong use, limitation
 * and counterplay. This check fails if any roster slot is missing that guidance,
 * if two slots share wording, or if a line is long enough to overrun the panel.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEchoesSelectionGuidanceTest,
    "Echoes.Runtime.UI.SelectionGuidanceCoverage",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::ClientContext |
        EAutomationTestFlags::EngineFilter)

bool FEchoesSelectionGuidanceTest::RunTest(const FString& Parameters)
{
    (void)Parameters;

    // A selection line sits in the bottom console beside health telemetry, so an
    // overlong sentence would push the rest of the card out of view.
    constexpr int32 MaxGuidanceLength = 160;

    TMap<FString, FString> SeenPurposes;
    int32 CoveredSlots = 0;

    for (const Faction FactionValue : GuidanceFactions)
    {
        for (const EntityType Type : GuidanceRosterTypes)
        {
            const FString Slot = SlotName(FactionValue, Type);
            const FEchoesRosterGuidance Guidance =
                FEchoesFieldHudModel::RosterGuidance(FactionValue, Type);

            // The catalog's own role is a schema token the content validator
            // depends on. Surfacing it would print "HEADQUARTERS DROPOFF" to the
            // player, so the panel must use an authored role instead.
            const FString DisplayRole = Guidance.DisplayRole.ToString();
            if (Guidance.DisplayRole.IsEmptyOrWhitespace())
            {
                AddError(FString::Printf(
                    TEXT("SPEC-HUD-003: %s has no authored display role."), *Slot));
            }
            else if (DisplayRole.Contains(TEXT("_")))
            {
                AddError(FString::Printf(
                    TEXT("SPEC-HUD-003: %s shows the raw schema token \"%s\" as its role."),
                    *Slot, *DisplayRole));
            }
            if (Guidance.Name.IsEmptyOrWhitespace())
            {
                AddError(FString::Printf(
                    TEXT("%s has no canonical name for a caller without a catalog."), *Slot));
            }

            const TArray<TPair<FString, FText>> Fields = {
                {TEXT("purpose"), Guidance.Purpose},
                {TEXT("strong use"), Guidance.StrongUse},
                {TEXT("limitation"), Guidance.Limitation},
                {TEXT("counterplay"), Guidance.Counterplay},
            };

            for (const TPair<FString, FText>& Field : Fields)
            {
                if (Field.Value.IsEmptyOrWhitespace())
                {
                    AddError(FString::Printf(
                        TEXT("SPEC-HUD-003: %s has no %s guidance."),
                        *Slot, *Field.Key));
                    continue;
                }
                const FString Text = Field.Value.ToString();
                if (Text.Len() > MaxGuidanceLength)
                {
                    AddError(FString::Printf(
                        TEXT("SPEC-HUD-003: %s %s is %d characters, over the %d the panel can show."),
                        *Slot, *Field.Key, Text.Len(), MaxGuidanceLength));
                }
                if (!Text.EndsWith(TEXT(".")))
                {
                    AddError(FString::Printf(
                        TEXT("SPEC-HUD-003: %s %s is not a complete sentence."), *Slot, *Field.Key));
                }
            }

            // Distinct roster entries must not share wording; a duplicate means one
            // entity is describing another, which would mislead the player.
            if (!Guidance.Purpose.IsEmptyOrWhitespace())
            {
                const FString Purpose = Guidance.Purpose.ToString();
                if (const FString* Existing = SeenPurposes.Find(Purpose))
                {
                    AddError(FString::Printf(
                        TEXT("SPEC-HUD-003: %s repeats the purpose already used by %s."),
                        *Slot, **Existing));
                }
                else
                {
                    SeenPurposes.Add(Purpose, Slot);
                }
            }

            ++CoveredSlots;
        }
    }

    TestEqual(TEXT("Every faction roster slot was checked"), CoveredSlots, 24);

    for (const Faction FactionValue : GuidanceFactions)
    {
        for (const EntityType Type : GuidanceNeutralTypes)
        {
            const FEchoesRosterGuidance Guidance =
                FEchoesFieldHudModel::RosterGuidance(FactionValue, Type);
            TestTrue(
                FString::Printf(TEXT("%s is a world object and carries no roster guidance"),
                    *SlotName(FactionValue, Type)),
                Guidance.Name.IsEmpty() && Guidance.DisplayRole.IsEmpty() &&
                    Guidance.Purpose.IsEmpty() && Guidance.StrongUse.IsEmpty() &&
                    Guidance.Limitation.IsEmpty() && Guidance.Counterplay.IsEmpty());
        }
    }

    return !HasAnyErrors();
}

#endif

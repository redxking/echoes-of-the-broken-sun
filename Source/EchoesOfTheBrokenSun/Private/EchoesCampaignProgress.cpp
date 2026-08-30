#include "EchoesCampaignProgress.h"

#include "EchoesBrokenSunMissionModel.h"

#include "HAL/FileManager.h"
#include "Misc/Crc.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace
{
constexpr uint8 CampaignMagic[] = {
    'E', 'C', 'H', 'O', 'C', 'P', 'G', '1'};
constexpr int32 HeaderSize = 12;
constexpr int32 LegacyRecordSize = 24;
constexpr int32 RecordSize = 27;
constexpr int32 ChecksumSize = 4;
constexpr uint8 AllWellChoicesMask = 0x07;
constexpr uint8 AllFinalResolutionsMask = 0x0F;
constexpr uint32 FutureThatWonMinimumSnapshotVersion = 21;
constexpr uint32 AssemblyOfTheMissingMinimumSnapshotVersion = 21;
constexpr uint32 SeveralVoicesOneCommandMinimumSnapshotVersion = 22;
constexpr uint32 BrokenSunMinimumSnapshotVersion = 22;
constexpr uint8 PrologueCompletionFacts =
    static_cast<uint8>(EEchoesCampaignDecisionFact::ArchiveRecovered) |
    static_cast<uint8>(EEchoesCampaignDecisionFact::CarrierEvacuated) |
    static_cast<uint8>(EEchoesCampaignDecisionFact::LocalCoreSurvived) |
    static_cast<uint8>(EEchoesCampaignDecisionFact::FutureWellControlled);
constexpr uint8 SevenAccountsCompletionFacts =
    static_cast<uint8>(EEchoesSevenAccountsCompletionFact::WaystoneRootedAtAnchor) |
    static_cast<uint8>(EEchoesSevenAccountsCompletionFact::MemoryBearerArrived) |
    static_cast<uint8>(EEchoesSevenAccountsCompletionFact::LocalCoreSurvived) |
    static_cast<uint8>(EEchoesSevenAccountsCompletionFact::PriorDecisionConsumed);
constexpr uint8 CityReserveCompletionFacts =
    static_cast<uint8>(EEchoesCityReserveCompletionFact::LifeSupportPowered) |
    static_cast<uint8>(EEchoesCityReserveCompletionFact::TransitPowered) |
    static_cast<uint8>(EEchoesCityReserveCompletionFact::ArchivePowered) |
    static_cast<uint8>(EEchoesCityReserveCompletionFact::LocalCoreSurvived) |
    static_cast<uint8>(EEchoesCityReserveCompletionFact::PriorLedgerConsumed);
constexpr uint8 UnburiedRoadCompletionFacts =
    static_cast<uint8>(EEchoesUnburiedRoadCompletionFact::WaystoneRootedAtRoadhead) |
    static_cast<uint8>(EEchoesUnburiedRoadCompletionFact::ListeningSpineRaised) |
    static_cast<uint8>(EEchoesUnburiedRoadCompletionFact::MemoryShardRecovered) |
    static_cast<uint8>(EEchoesUnburiedRoadCompletionFact::LocalCoreSurvived) |
    static_cast<uint8>(EEchoesUnburiedRoadCompletionFact::PriorLedgerConsumed);
constexpr uint8 TermsOfContinuanceCompletionFacts =
    static_cast<uint8>(EEchoesTermsOfContinuanceCompletionFact::MeridianRelaySynchronized) |
    static_cast<uint8>(EEchoesTermsOfContinuanceCompletionFact::KharuunSpineSynchronized) |
    static_cast<uint8>(EEchoesTermsOfContinuanceCompletionFact::ContinuanceWindowHeld) |
    static_cast<uint8>(EEchoesTermsOfContinuanceCompletionFact::BothWitnessesExtracted) |
    static_cast<uint8>(EEchoesTermsOfContinuanceCompletionFact::LocalCoreSurvived) |
    static_cast<uint8>(EEchoesTermsOfContinuanceCompletionFact::PriorLedgerConsumed);
constexpr uint8 NamesWithoutBirthsCompletionFacts =
    static_cast<uint8>(EEchoesNamesWithoutBirthsCompletionFact::CensusEvidenceLocated) |
    static_cast<uint8>(EEchoesNamesWithoutBirthsCompletionFact::ArchivePowered) |
    static_cast<uint8>(EEchoesNamesWithoutBirthsCompletionFact::BothCiviliansSheltered) |
    static_cast<uint8>(EEchoesNamesWithoutBirthsCompletionFact::EvidenceExtracted) |
    static_cast<uint8>(EEchoesNamesWithoutBirthsCompletionFact::LocalCoreSurvived) |
    static_cast<uint8>(EEchoesNamesWithoutBirthsCompletionFact::PriorLedgerConsumed);
constexpr uint8 ShapeOfSilenceCompletionFacts =
    static_cast<uint8>(EEchoesShapeOfSilenceCompletionFact::WaystoneRootedAtListeningAnchor) |
    static_cast<uint8>(EEchoesShapeOfSilenceCompletionFact::ListeningSpineRaised) |
    static_cast<uint8>(EEchoesShapeOfSilenceCompletionFact::BothMemoryWitnessesPositioned) |
    static_cast<uint8>(EEchoesShapeOfSilenceCompletionFact::OruunReachedConfluence) |
    static_cast<uint8>(EEchoesShapeOfSilenceCompletionFact::LocalCoreSurvived) |
    static_cast<uint8>(EEchoesShapeOfSilenceCompletionFact::PriorLedgerConsumed);
constexpr uint8 ShapeBesideUsCompletionFacts =
    static_cast<uint8>(EEchoesShapeBesideUsCompletionFact::FirstEchoObserved) |
    static_cast<uint8>(EEchoesShapeBesideUsCompletionFact::EchoRelayRaised) |
    static_cast<uint8>(EEchoesShapeBesideUsCompletionFact::BothStatesTraversed) |
    static_cast<uint8>(EEchoesShapeBesideUsCompletionFact::NemeConvergenceReached) |
    static_cast<uint8>(EEchoesShapeBesideUsCompletionFact::LocalCoreSurvived) |
    static_cast<uint8>(EEchoesShapeBesideUsCompletionFact::PriorLedgerConsumed);
constexpr uint8 ReserveAuthorityDistrictFacts =
    static_cast<uint8>(EEchoesReserveAuthorityCompletionFact::LifeSupportPowered) |
    static_cast<uint8>(EEchoesReserveAuthorityCompletionFact::TransitPowered) |
    static_cast<uint8>(EEchoesReserveAuthorityCompletionFact::ArchivePowered);
constexpr uint8 ReserveAuthorityCommonCompletionFacts =
    static_cast<uint8>(EEchoesReserveAuthorityCompletionFact::ReserveAuthoritySecured) |
    static_cast<uint8>(EEchoesReserveAuthorityCompletionFact::DeferredDistrictReached) |
    static_cast<uint8>(EEchoesReserveAuthorityCompletionFact::LocalCoreSurvived) |
    static_cast<uint8>(EEchoesReserveAuthorityCompletionFact::PriorLedgerConsumed);
constexpr uint8 ChoirAtLumeReachCompletionFacts =
    static_cast<uint8>(EEchoesChoirAtLumeReachCompletionFact::ContactEstablished) |
    static_cast<uint8>(EEchoesChoirAtLumeReachCompletionFact::DeferredLiabilityResolved) |
    static_cast<uint8>(EEchoesChoirAtLumeReachCompletionFact::BothAnchorsRaised) |
    static_cast<uint8>(EEchoesChoirAtLumeReachCompletionFact::WellChoiceCommitted) |
    static_cast<uint8>(EEchoesChoirAtLumeReachCompletionFact::BranchResolutionCompleted) |
    static_cast<uint8>(EEchoesChoirAtLumeReachCompletionFact::OruunSurvived) |
    static_cast<uint8>(EEchoesChoirAtLumeReachCompletionFact::LocalCoreSurvived) |
    static_cast<uint8>(EEchoesChoirAtLumeReachCompletionFact::PriorLedgerConsumed);
constexpr uint8 NoNeutralLedgerCompletionFacts =
    static_cast<uint8>(EEchoesNoNeutralLedgerCompletionFact::InheritedRouteSecured) |
    static_cast<uint8>(EEchoesNoNeutralLedgerCompletionFact::DistrictPairIntegrated) |
    static_cast<uint8>(EEchoesNoNeutralLedgerCompletionFact::BothEvidenceChannelsAttested) |
    static_cast<uint8>(EEchoesNoNeutralLedgerCompletionFact::RecordedProtocolApplied) |
    static_cast<uint8>(EEchoesNoNeutralLedgerCompletionFact::CoalitionRallied) |
    static_cast<uint8>(EEchoesNoNeutralLedgerCompletionFact::OruunSurvived) |
    static_cast<uint8>(EEchoesNoNeutralLedgerCompletionFact::LocalCoreSurvived) |
    static_cast<uint8>(EEchoesNoNeutralLedgerCompletionFact::PriorLedgerConsumed);
constexpr uint8 FutureThatWonCompletionFacts =
    static_cast<uint8>(EEchoesFutureThatWonCompletionFact::PriorElevenRecordLedgerConsumed) |
    static_cast<uint8>(EEchoesFutureThatWonCompletionFact::RecordedLumeProtocolBound) |
    static_cast<uint8>(EEchoesFutureThatWonCompletionFact::BothRecordedDistrictInputsVerified) |
    static_cast<uint8>(EEchoesFutureThatWonCompletionFact::IndependentPublicReadbackEstablished) |
    static_cast<uint8>(EEchoesFutureThatWonCompletionFact::RecordedProtocolActivated) |
    static_cast<uint8>(EEchoesFutureThatWonCompletionFact::StabilityWindowHeld) |
    static_cast<uint8>(EEchoesFutureThatWonCompletionFact::BothDistrictReadbacksObserved) |
    static_cast<uint8>(EEchoesFutureThatWonCompletionFact::LocalCoreSurvived);
constexpr uint8 AssemblyOfTheMissingCompletionFacts =
    static_cast<uint8>(EEchoesAssemblyOfTheMissingCompletionFact::PriorTwelveRecordLedgerConsumed) |
    static_cast<uint8>(EEchoesAssemblyOfTheMissingCompletionFact::ExistingPlanProjectionBound) |
    static_cast<uint8>(EEchoesAssemblyOfTheMissingCompletionFact::RecordedLumeProtocolBound) |
    static_cast<uint8>(EEchoesAssemblyOfTheMissingCompletionFact::PriorPublicReceiptsBound) |
    static_cast<uint8>(EEchoesAssemblyOfTheMissingCompletionFact::PublicRecordReadbackEstablished) |
    static_cast<uint8>(EEchoesAssemblyOfTheMissingCompletionFact::CrownfallIndexLinked) |
    static_cast<uint8>(EEchoesAssemblyOfTheMissingCompletionFact::IndependentAssemblyObserved) |
    static_cast<uint8>(EEchoesAssemblyOfTheMissingCompletionFact::LocalCoreSurvived);
constexpr uint8 SeveralVoicesOneCommandCompletionFacts =
    static_cast<uint8>(EEchoesSeveralVoicesOneCommandCompletionFact::PriorThirteenRecordLedgerConsumed) |
    static_cast<uint8>(EEchoesSeveralVoicesOneCommandCompletionFact::ChoirCommandAuthorityEstablished) |
    static_cast<uint8>(EEchoesSeveralVoicesOneCommandCompletionFact::HeldAlternativesResearched) |
    static_cast<uint8>(EEchoesSeveralVoicesOneCommandCompletionFact::IncompatibleVoicesResolved) |
    static_cast<uint8>(EEchoesSeveralVoicesOneCommandCompletionFact::SharedResolutionResearched) |
    static_cast<uint8>(EEchoesSeveralVoicesOneCommandCompletionFact::PhaseAnchorRaised) |
    static_cast<uint8>(EEchoesSeveralVoicesOneCommandCompletionFact::CrisisWindowHeld) |
    static_cast<uint8>(EEchoesSeveralVoicesOneCommandCompletionFact::LocalCoreSurvived);
constexpr uint8 BrokenSunCompletionFacts =
    static_cast<uint8>(EEchoesBrokenSunCompletionFact::PriorFourteenRecordLedgerConsumed) |
    static_cast<uint8>(EEchoesBrokenSunCompletionFact::CrownfallApproachSecured) |
    static_cast<uint8>(EEchoesBrokenSunCompletionFact::AccordAssemblyEstablished) |
    static_cast<uint8>(EEchoesBrokenSunCompletionFact::FinalResolutionCommitted) |
    static_cast<uint8>(EEchoesBrokenSunCompletionFact::ResolutionConduitRaised) |
    static_cast<uint8>(EEchoesBrokenSunCompletionFact::ResolutionWindowHeld) |
    static_cast<uint8>(EEchoesBrokenSunCompletionFact::NamedWitnessesSurvived) |
    static_cast<uint8>(EEchoesBrokenSunCompletionFact::LocalCoreSurvived);

void AppendU8(TArray<uint8>& Bytes, uint8 Value)
{
    Bytes.Add(Value);
}

void AppendU16(TArray<uint8>& Bytes, uint16 Value)
{
    Bytes.Add(static_cast<uint8>(Value));
    Bytes.Add(static_cast<uint8>(Value >> 8));
}

void AppendU32(TArray<uint8>& Bytes, uint32 Value)
{
    for (int32 ByteIndex = 0; ByteIndex < 4; ++ByteIndex)
    {
        Bytes.Add(static_cast<uint8>(Value >> (ByteIndex * 8)));
    }
}

void AppendU64(TArray<uint8>& Bytes, uint64 Value)
{
    for (int32 ByteIndex = 0; ByteIndex < 8; ++ByteIndex)
    {
        Bytes.Add(static_cast<uint8>(Value >> (ByteIndex * 8)));
    }
}

bool ReadU8(const TArray<uint8>& Bytes, int32& Offset, uint8& OutValue)
{
    if (!Bytes.IsValidIndex(Offset))
    {
        return false;
    }
    OutValue = Bytes[Offset++];
    return true;
}

bool ReadU16(const TArray<uint8>& Bytes, int32& Offset, uint16& OutValue)
{
    if (Offset < 0 || Offset + 2 > Bytes.Num())
    {
        return false;
    }
    OutValue = static_cast<uint16>(Bytes[Offset]) |
        static_cast<uint16>(Bytes[Offset + 1]) << 8;
    Offset += 2;
    return true;
}

bool ReadU32(const TArray<uint8>& Bytes, int32& Offset, uint32& OutValue)
{
    if (Offset < 0 || Offset + 4 > Bytes.Num())
    {
        return false;
    }
    OutValue = 0;
    for (int32 ByteIndex = 0; ByteIndex < 4; ++ByteIndex)
    {
        OutValue |= static_cast<uint32>(Bytes[Offset + ByteIndex])
            << (ByteIndex * 8);
    }
    Offset += 4;
    return true;
}

bool ReadU64(const TArray<uint8>& Bytes, int32& Offset, uint64& OutValue)
{
    if (Offset < 0 || Offset + 8 > Bytes.Num())
    {
        return false;
    }
    OutValue = 0;
    for (int32 ByteIndex = 0; ByteIndex < 8; ++ByteIndex)
    {
        OutValue |= static_cast<uint64>(Bytes[Offset + ByteIndex])
            << (ByteIndex * 8);
    }
    Offset += 8;
    return true;
}

uint8 ChoiceMask(echoes::sim::FutureWellChoice Choice)
{
    switch (Choice)
    {
        case echoes::sim::FutureWellChoice::Harvest: return 1 << 0;
        case echoes::sim::FutureWellChoice::Preserve: return 1 << 1;
        case echoes::sim::FutureWellChoice::Reshape: return 1 << 2;
        default: return 0;
    }
}

bool ValidateRecord(
    const FEchoesCampaignDecisionRecord& Record,
    FString& OutError)
{
    if (Record.Mission != EEchoesCampaignMissionId::WhatTheLedgerKeeps &&
        Record.Mission != EEchoesCampaignMissionId::SevenAccountsOfRain &&
        Record.Mission != EEchoesCampaignMissionId::ACityOnReserve &&
        Record.Mission != EEchoesCampaignMissionId::TheUnburiedRoad &&
        Record.Mission != EEchoesCampaignMissionId::TermsOfContinuance &&
        Record.Mission != EEchoesCampaignMissionId::NamesWithoutBirths &&
        Record.Mission != EEchoesCampaignMissionId::TheShapeOfSilence &&
        Record.Mission != EEchoesCampaignMissionId::TheShapeBesideUs &&
        Record.Mission != EEchoesCampaignMissionId::ReserveAuthority &&
        Record.Mission != EEchoesCampaignMissionId::ChoirAtLumeReach &&
        Record.Mission != EEchoesCampaignMissionId::NoNeutralLedger &&
        Record.Mission != EEchoesCampaignMissionId::TheFutureThatWon &&
        Record.Mission != EEchoesCampaignMissionId::AssemblyOfTheMissing &&
        Record.Mission != EEchoesCampaignMissionId::SeveralVoicesOneCommand &&
        Record.Mission != EEchoesCampaignMissionId::TheBrokenSun)
    {
        OutError = TEXT("[CAMPAIGN_UNKNOWN_MISSION] The campaign record names an unsupported mission.");
        return false;
    }
    const uint8 SelectedChoice = ChoiceMask(Record.WellChoice);
    if (SelectedChoice == 0 ||
        (Record.AvailableWellChoices & SelectedChoice) == 0 ||
        (Record.AvailableWellChoices & ~AllWellChoicesMask) != 0)
    {
        OutError = TEXT("[CAMPAIGN_INVALID_WELL_DECISION] The recorded Well decision is inconsistent.");
        return false;
    }
    if (Record.Mission == EEchoesCampaignMissionId::ChoirAtLumeReach &&
        Record.AvailableWellChoices != AllWellChoicesMask)
    {
        OutError = TEXT("[CAMPAIGN_INVALID_WELL_DECISION] The Lume Reach record must preserve all three offered Well protocols.");
        return false;
    }
    if ((Record.Mission == EEchoesCampaignMissionId::NoNeutralLedger ||
         Record.Mission == EEchoesCampaignMissionId::TheFutureThatWon ||
         Record.Mission == EEchoesCampaignMissionId::AssemblyOfTheMissing ||
         Record.Mission == EEchoesCampaignMissionId::SeveralVoicesOneCommand ||
         Record.Mission == EEchoesCampaignMissionId::TheBrokenSun) &&
        Record.AvailableWellChoices != SelectedChoice)
    {
        OutError = TEXT("[CAMPAIGN_INVALID_WELL_DECISION] The downstream receipt must retain only the recorded Lume protocol that was applied.");
        return false;
    }
    if (Record.Mission == EEchoesCampaignMissionId::TheBrokenSun)
    {
        const uint8 ResolutionMask =
            FEchoesBrokenSunMissionModel::ResolutionMask(
                Record.FinalResolution);
        if (ResolutionMask == 0 ||
            (Record.AvailableFinalResolutions & ResolutionMask) == 0 ||
            (Record.AvailableFinalResolutions &
             ~AllFinalResolutionsMask) != 0 ||
            Record.FinalPlanKey >= 27)
        {
            OutError = TEXT("[CAMPAIGN_INVALID_FINAL_RESOLUTION] The final resolution, eligibility mask, or inherited plan key is inconsistent.");
            return false;
        }
    }
    else if (Record.FinalResolution != EEchoesFinalResolution::None ||
             Record.AvailableFinalResolutions != 0 ||
             Record.FinalPlanKey != 0xFF)
    {
        OutError = TEXT("[CAMPAIGN_UNEXPECTED_FINAL_RESOLUTION] Only Mission 15 may carry a final campaign resolution.");
        return false;
    }
    const uint8 RequiredFacts =
        Record.Mission == EEchoesCampaignMissionId::WhatTheLedgerKeeps
            ? PrologueCompletionFacts
        : Record.Mission == EEchoesCampaignMissionId::SevenAccountsOfRain
            ? SevenAccountsCompletionFacts
        : Record.Mission == EEchoesCampaignMissionId::ACityOnReserve
            ? CityReserveCompletionFacts
        : Record.Mission == EEchoesCampaignMissionId::TheUnburiedRoad
            ? UnburiedRoadCompletionFacts
        : Record.Mission == EEchoesCampaignMissionId::TermsOfContinuance
            ? TermsOfContinuanceCompletionFacts
        : Record.Mission == EEchoesCampaignMissionId::NamesWithoutBirths
            ? NamesWithoutBirthsCompletionFacts
        : Record.Mission == EEchoesCampaignMissionId::TheShapeOfSilence
            ? ShapeOfSilenceCompletionFacts
        : Record.Mission == EEchoesCampaignMissionId::TheShapeBesideUs
            ? ShapeBesideUsCompletionFacts
        : Record.Mission == EEchoesCampaignMissionId::ReserveAuthority
            ? ReserveAuthorityCommonCompletionFacts
        : Record.Mission == EEchoesCampaignMissionId::ChoirAtLumeReach
            ? ChoirAtLumeReachCompletionFacts
        : Record.Mission == EEchoesCampaignMissionId::NoNeutralLedger
            ? NoNeutralLedgerCompletionFacts
        : Record.Mission == EEchoesCampaignMissionId::TheFutureThatWon
            ? FutureThatWonCompletionFacts
        : Record.Mission == EEchoesCampaignMissionId::AssemblyOfTheMissing
            ? AssemblyOfTheMissingCompletionFacts
        : Record.Mission == EEchoesCampaignMissionId::SeveralVoicesOneCommand
            ? SeveralVoicesOneCommandCompletionFacts
            : BrokenSunCompletionFacts;
    const uint8 ReserveDistricts =
        Record.VerifiedFacts & ReserveAuthorityDistrictFacts;
    const bool bValidReserveAllocation =
        Record.Mission != EEchoesCampaignMissionId::ReserveAuthority ||
        (ReserveDistricts ==
             (static_cast<uint8>(
                  EEchoesReserveAuthorityCompletionFact::LifeSupportPowered) |
              static_cast<uint8>(
                  EEchoesReserveAuthorityCompletionFact::TransitPowered)) ||
         ReserveDistricts ==
             (static_cast<uint8>(
                  EEchoesReserveAuthorityCompletionFact::LifeSupportPowered) |
              static_cast<uint8>(
                  EEchoesReserveAuthorityCompletionFact::ArchivePowered)) ||
         ReserveDistricts ==
             (static_cast<uint8>(
                  EEchoesReserveAuthorityCompletionFact::TransitPowered) |
              static_cast<uint8>(
                  EEchoesReserveAuthorityCompletionFact::ArchivePowered)));
    const uint8 AllowedFacts =
        Record.Mission == EEchoesCampaignMissionId::ReserveAuthority
            ? ReserveAuthorityCommonCompletionFacts |
                  ReserveAuthorityDistrictFacts
            : RequiredFacts;
    if ((Record.VerifiedFacts & RequiredFacts) != RequiredFacts ||
        (Record.VerifiedFacts & ~AllowedFacts) != 0 ||
        !bValidReserveAllocation)
    {
        OutError = TEXT("[CAMPAIGN_UNVERIFIED_COMPLETION] The record does not prove the mission completion contract.");
        return false;
    }
    if (Record.SimulationSnapshotVersion == 0 ||
        Record.CompletionTick == 0 || Record.FinalStateChecksum == 0)
    {
        OutError = TEXT("[CAMPAIGN_INVALID_PROVENANCE] The record lacks deterministic completion provenance.");
        return false;
    }
    if (Record.Mission == EEchoesCampaignMissionId::TheFutureThatWon &&
        (Record.SimulationSnapshotVersion <
             FutureThatWonMinimumSnapshotVersion ||
         Record.SimulationSnapshotVersion > echoes::sim::kSnapshotVersion))
    {
        OutError = FString::Printf(
            TEXT("[CAMPAIGN_SNAPSHOT_VERSION_REQUIRED] Mission 12 requires activation provenance from snapshot schema 21 through supported schema %u."),
            echoes::sim::kSnapshotVersion);
        return false;
    }
    if (Record.Mission == EEchoesCampaignMissionId::AssemblyOfTheMissing &&
        (Record.SimulationSnapshotVersion <
             AssemblyOfTheMissingMinimumSnapshotVersion ||
         Record.SimulationSnapshotVersion > echoes::sim::kSnapshotVersion))
    {
        OutError = FString::Printf(
            TEXT("[CAMPAIGN_SNAPSHOT_VERSION_REQUIRED] Mission 13 requires public-assembly provenance from snapshot schema 21 through supported schema %u."),
            echoes::sim::kSnapshotVersion);
        return false;
    }
    if (Record.Mission == EEchoesCampaignMissionId::SeveralVoicesOneCommand &&
        (Record.SimulationSnapshotVersion <
             SeveralVoicesOneCommandMinimumSnapshotVersion ||
         Record.SimulationSnapshotVersion > echoes::sim::kSnapshotVersion))
    {
        OutError = FString::Printf(
            TEXT("[CAMPAIGN_SNAPSHOT_VERSION_REQUIRED] Mission 14 requires native Hollow Choir provenance from snapshot schema 22 through supported schema %u."),
            echoes::sim::kSnapshotVersion);
        return false;
    }
    if (Record.Mission == EEchoesCampaignMissionId::TheBrokenSun &&
        (Record.SimulationSnapshotVersion < BrokenSunMinimumSnapshotVersion ||
         Record.SimulationSnapshotVersion > echoes::sim::kSnapshotVersion))
    {
        OutError = FString::Printf(
            TEXT("[CAMPAIGN_SNAPSHOT_VERSION_REQUIRED] Mission 15 requires final-operation provenance from snapshot schema 22 through supported schema %u."),
            echoes::sim::kSnapshotVersion);
        return false;
    }
    return true;
}

bool ValidateNoNeutralLedgerSequence(
    const FEchoesCampaignProgress& Progress,
    FString& OutError)
{
    const FEchoesCampaignDecisionRecord* Alliance =
        Progress.FindDecision(EEchoesCampaignMissionId::NoNeutralLedger);
    const FEchoesCampaignDecisionRecord* Restoration =
        Progress.FindDecision(EEchoesCampaignMissionId::TheFutureThatWon);
    const FEchoesCampaignDecisionRecord* Assembly =
        Progress.FindDecision(EEchoesCampaignMissionId::AssemblyOfTheMissing);
    const FEchoesCampaignDecisionRecord* Voices =
        Progress.FindDecision(
            EEchoesCampaignMissionId::SeveralVoicesOneCommand);
    const FEchoesCampaignDecisionRecord* Finale =
        Progress.FindDecision(EEchoesCampaignMissionId::TheBrokenSun);
    if (Alliance == nullptr && Restoration == nullptr && Assembly == nullptr &&
        Voices == nullptr && Finale == nullptr)
    {
        return true;
    }
    if (Alliance == nullptr)
    {
        OutError = Finale != nullptr
            ? TEXT("[CAMPAIGN_FINALE_LEDGER_INVALID] Mission 15 requires the accepted Missions 11 through 14 receipts.")
        : Voices != nullptr
            ? TEXT("[CAMPAIGN_VOICES_LEDGER_INVALID] Mission 14 requires the accepted Missions 11 through 13 receipts.")
        : Assembly != nullptr
            ? TEXT("[CAMPAIGN_ASSEMBLY_LEDGER_INVALID] Mission 13 requires the accepted Mission 11 and Mission 12 receipts.")
            : TEXT("[CAMPAIGN_RESTORATION_LEDGER_INVALID] Mission 12 requires the accepted Mission 11 receipt.");
        return false;
    }
    if ((Assembly != nullptr || Voices != nullptr || Finale != nullptr) &&
        Restoration == nullptr)
    {
        OutError = TEXT("[CAMPAIGN_ASSEMBLY_LEDGER_INVALID] Mission 13 requires the accepted Mission 12 readback receipt.");
        return false;
    }
    if ((Voices != nullptr || Finale != nullptr) && Assembly == nullptr)
    {
        OutError = TEXT("[CAMPAIGN_VOICES_LEDGER_INVALID] Mission 14 requires the accepted Mission 13 assembly receipt.");
        return false;
    }
    if (Finale != nullptr && Voices == nullptr)
    {
        OutError = TEXT("[CAMPAIGN_FINALE_LEDGER_INVALID] Mission 15 requires the accepted Mission 14 Choir-command receipt.");
        return false;
    }
    const int32 ExpectedRecords = Finale != nullptr
        ? 15
        : Voices != nullptr
        ? 14
        : Assembly != nullptr
        ? 13
        : Restoration != nullptr ? 12 : 11;
    if (Progress.Decisions.Num() != ExpectedRecords)
    {
        OutError = Finale != nullptr
            ? TEXT("[CAMPAIGN_FINALE_LEDGER_INVALID] Mission 15 requires exactly fourteen prior records and one final-resolution record.")
        : Voices != nullptr
            ? TEXT("[CAMPAIGN_VOICES_LEDGER_INVALID] Mission 14 requires exactly thirteen prior records and one Choir-command record.")
        : Assembly != nullptr
            ? TEXT("[CAMPAIGN_ASSEMBLY_LEDGER_INVALID] Mission 13 requires exactly twelve prior records and one public-assembly record.")
        : Restoration != nullptr
            ? TEXT("[CAMPAIGN_RESTORATION_LEDGER_INVALID] Mission 12 requires exactly eleven prior records and one restoration record.")
            : TEXT("[CAMPAIGN_ALLIANCE_LEDGER_INVALID] Mission 11 requires exactly ten prior records and one alliance record.");
        return false;
    }
    for (int32 Index = 0; Index < ExpectedRecords; ++Index)
    {
        if (static_cast<uint8>(Progress.Decisions[Index].Mission) !=
            static_cast<uint8>(Index + 1))
        {
            OutError = Finale != nullptr
                ? TEXT("[CAMPAIGN_FINALE_LEDGER_ORDER] Mission 15 requires the exact ordered M01-M14 record chain.")
            : Voices != nullptr
                ? TEXT("[CAMPAIGN_VOICES_LEDGER_ORDER] Mission 14 requires the exact ordered M01-M13 record chain.")
            : Assembly != nullptr
                ? TEXT("[CAMPAIGN_ASSEMBLY_LEDGER_ORDER] Mission 13 requires the exact ordered M01-M12 record chain.")
            : Restoration != nullptr
                ? TEXT("[CAMPAIGN_RESTORATION_LEDGER_ORDER] Mission 12 requires the exact ordered M01-M11 record chain.")
                : TEXT("[CAMPAIGN_ALLIANCE_LEDGER_ORDER] Mission 11 requires the exact ordered M01-M10 record chain.");
            return false;
        }
    }
    const FEchoesCampaignDecisionRecord& Founding = Progress.Decisions[0];
    for (int32 Index = 1; Index <= 8; ++Index)
    {
        if (Progress.Decisions[Index].WellChoice != Founding.WellChoice)
        {
            OutError = TEXT("[CAMPAIGN_ALLIANCE_LEDGER_BRANCH] Missions 02-09 must retain the founding Well doctrine.");
            return false;
        }
    }
    const FEchoesCampaignDecisionRecord& Lume = Progress.Decisions[9];
    if (Alliance->WellChoice != Lume.WellChoice ||
        Alliance->AvailableWellChoices != ChoiceMask(Lume.WellChoice))
    {
        OutError = TEXT("[CAMPAIGN_ALLIANCE_LUME_PROTOCOL] Mission 11 does not retain the independently recorded Mission 10 protocol.");
        return false;
    }
    if (Restoration != nullptr &&
        (Restoration->WellChoice != Alliance->WellChoice ||
         Restoration->AvailableWellChoices !=
             ChoiceMask(Alliance->WellChoice)))
    {
        OutError = TEXT("[CAMPAIGN_RESTORATION_LUME_PROTOCOL] Mission 12 does not retain the exact Mission 11 protocol receipt.");
        return false;
    }
    if (Assembly != nullptr &&
        (Assembly->WellChoice != Restoration->WellChoice ||
         Assembly->AvailableWellChoices !=
             ChoiceMask(Restoration->WellChoice)))
    {
        OutError = TEXT("[CAMPAIGN_ASSEMBLY_LUME_PROTOCOL] Mission 13 does not retain the exact Mission 12 protocol receipt.");
        return false;
    }
    if (Voices != nullptr &&
        (Voices->WellChoice != Assembly->WellChoice ||
         Voices->AvailableWellChoices !=
             ChoiceMask(Assembly->WellChoice)))
    {
        OutError = TEXT("[CAMPAIGN_VOICES_LUME_PROTOCOL] Mission 14 does not retain the exact Mission 13 protocol receipt.");
        return false;
    }
    if (Finale != nullptr &&
        (Finale->WellChoice != Voices->WellChoice ||
         Finale->AvailableWellChoices !=
             ChoiceMask(Voices->WellChoice)))
    {
        OutError = TEXT("[CAMPAIGN_FINALE_LUME_PROTOCOL] Mission 15 does not retain the exact Mission 14 protocol receipt.");
        return false;
    }
    if (Finale != nullptr)
    {
        const FEchoesCampaignDecisionRecord& Reserve = Progress.Decisions[8];
        FEchoesBrokenSunPlan Plan;
        if (!FEchoesBrokenSunMissionModel::TryPlanForLedger(
                Founding.WellChoice,
                Reserve.VerifiedFacts,
                Finale->WellChoice,
                Plan) ||
            Finale->AvailableFinalResolutions !=
                Plan.AvailableFinalResolutions ||
            Finale->FinalPlanKey != Plan.StablePlanKey ||
            !FEchoesBrokenSunMissionModel::IsResolutionAvailable(
                Plan,
                Finale->FinalResolution))
        {
            OutError = TEXT("[CAMPAIGN_FINALE_PROJECTION_INVALID] Mission 15 does not retain the earned ending set and exact inherited plan.");
            return false;
        }
    }
    return true;
}

bool TryLoadOne(
    const FString& Path,
    FEchoesCampaignProgress& OutProgress,
    FString& OutFailure)
{
    TArray<uint8> Bytes;
    if (!FFileHelper::LoadFileToArray(Bytes, *Path))
    {
        OutFailure = TEXT("file unavailable");
        return false;
    }
    return FEchoesCampaignProgressStore::Decode(
        Bytes,
        OutProgress,
        OutFailure);
}
}

const FEchoesCampaignDecisionRecord* FEchoesCampaignProgress::FindDecision(
    EEchoesCampaignMissionId Mission) const
{
    return Decisions.FindByPredicate(
        [Mission](const FEchoesCampaignDecisionRecord& Record)
        {
            return Record.Mission == Mission;
        });
}

EEchoesCampaignCommitStatus FEchoesCampaignProgress::AppendDecision(
    const FEchoesCampaignDecisionRecord& Record,
    FString& OutFeedback)
{
    if (!ValidateRecord(Record, OutFeedback))
    {
        return EEchoesCampaignCommitStatus::StorageFailure;
    }
    if (const FEchoesCampaignDecisionRecord* Existing =
            FindDecision(Record.Mission))
    {
        if (Existing->WellChoice == Record.WellChoice &&
            Existing->AvailableWellChoices == Record.AvailableWellChoices &&
            Existing->VerifiedFacts == Record.VerifiedFacts &&
            Existing->FinalResolution == Record.FinalResolution &&
            Existing->AvailableFinalResolutions ==
                Record.AvailableFinalResolutions &&
            Existing->FinalPlanKey == Record.FinalPlanKey)
        {
            OutFeedback = TEXT("CAMPAIGN LEDGER: this mission decision was already recorded.");
            return EEchoesCampaignCommitStatus::AlreadyRecorded;
        }
        OutFeedback = TEXT("CAMPAIGN LEDGER: replay outcome retained, but the original irreversible decision was not rewritten.");
        return EEchoesCampaignCommitStatus::ReplayConflict;
    }
    if (Decisions.Num() >= MaximumDecisionRecords)
    {
        OutFeedback = TEXT("[CAMPAIGN_RECORD_LIMIT] The campaign ledger cannot accept another decision.");
        return EEchoesCampaignCommitStatus::StorageFailure;
    }
    if (Record.Mission == EEchoesCampaignMissionId::NoNeutralLedger ||
        Record.Mission == EEchoesCampaignMissionId::TheFutureThatWon ||
        Record.Mission == EEchoesCampaignMissionId::AssemblyOfTheMissing ||
        Record.Mission == EEchoesCampaignMissionId::SeveralVoicesOneCommand ||
        Record.Mission == EEchoesCampaignMissionId::TheBrokenSun)
    {
        FEchoesCampaignProgress Candidate = *this;
        Candidate.Decisions.Add(Record);
        if (!ValidateNoNeutralLedgerSequence(Candidate, OutFeedback))
        {
            return EEchoesCampaignCommitStatus::StorageFailure;
        }
    }
    Decisions.Add(Record);
    OutFeedback = TEXT("CAMPAIGN LEDGER: mission consequence committed.");
    return EEchoesCampaignCommitStatus::Added;
}

FString FEchoesCampaignProgressStore::GetDefaultPath()
{
    return FPaths::Combine(
        FPaths::ProjectSavedDir(),
        TEXT("SaveGames"),
        TEXT("EchoesCampaignProgress.bin"));
}

bool FEchoesCampaignProgressStore::Encode(
    const FEchoesCampaignProgress& Progress,
    TArray<uint8>& OutBytes,
    FString& OutError)
{
    OutBytes.Reset();
    OutError.Reset();
    if (Progress.Decisions.Num() > FEchoesCampaignProgress::MaximumDecisionRecords)
    {
        OutError = TEXT("[CAMPAIGN_RECORD_LIMIT] Too many campaign records were supplied.");
        return false;
    }

    TSet<uint8> SeenMissions;
    for (const FEchoesCampaignDecisionRecord& Record : Progress.Decisions)
    {
        if (!ValidateRecord(Record, OutError))
        {
            return false;
        }
        const uint8 MissionValue = static_cast<uint8>(Record.Mission);
        if (SeenMissions.Contains(MissionValue))
        {
            OutError = TEXT("[CAMPAIGN_DUPLICATE_MISSION] The ledger contains more than one decision for a mission.");
            return false;
        }
        SeenMissions.Add(MissionValue);
    }
    if (!ValidateNoNeutralLedgerSequence(Progress, OutError))
    {
        return false;
    }

    OutBytes.Reserve(
        HeaderSize + Progress.Decisions.Num() * RecordSize + ChecksumSize);
    OutBytes.Append(CampaignMagic, UE_ARRAY_COUNT(CampaignMagic));
    AppendU16(OutBytes, FEchoesCampaignProgress::SchemaVersion);
    AppendU16(OutBytes, static_cast<uint16>(Progress.Decisions.Num()));
    for (const FEchoesCampaignDecisionRecord& Record : Progress.Decisions)
    {
        AppendU8(OutBytes, static_cast<uint8>(Record.Mission));
        AppendU8(OutBytes, static_cast<uint8>(Record.WellChoice));
        AppendU8(OutBytes, Record.AvailableWellChoices);
        AppendU8(OutBytes, Record.VerifiedFacts);
        AppendU8(
            OutBytes,
            static_cast<uint8>(Record.FinalResolution));
        AppendU8(OutBytes, Record.AvailableFinalResolutions);
        AppendU8(OutBytes, Record.FinalPlanKey);
        AppendU32(OutBytes, Record.SimulationSnapshotVersion);
        AppendU64(OutBytes, Record.CompletionTick);
        AppendU64(OutBytes, Record.FinalStateChecksum);
    }
    AppendU32(
        OutBytes,
        FCrc::MemCrc32(OutBytes.GetData(), OutBytes.Num()));
    return true;
}

bool FEchoesCampaignProgressStore::Decode(
    const TArray<uint8>& Bytes,
    FEchoesCampaignProgress& OutProgress,
    FString& OutError)
{
    OutError.Reset();
    if (Bytes.Num() < HeaderSize + ChecksumSize)
    {
        OutError = TEXT("[CAMPAIGN_TRUNCATED] The campaign ledger is incomplete.");
        return false;
    }
    for (int32 MagicIndex = 0;
         MagicIndex < UE_ARRAY_COUNT(CampaignMagic);
         ++MagicIndex)
    {
        if (Bytes[MagicIndex] != CampaignMagic[MagicIndex])
        {
            OutError = TEXT("[CAMPAIGN_MAGIC_MISMATCH] The file is not an Echoes campaign ledger.");
            return false;
        }
    }

    int32 ChecksumOffset = Bytes.Num() - ChecksumSize;
    uint32 StoredChecksum = 0;
    if (!ReadU32(Bytes, ChecksumOffset, StoredChecksum) ||
        StoredChecksum != FCrc::MemCrc32(Bytes.GetData(), Bytes.Num() - ChecksumSize))
    {
        OutError = TEXT("[CAMPAIGN_CHECKSUM_MISMATCH] The campaign ledger failed integrity validation.");
        return false;
    }

    int32 Offset = UE_ARRAY_COUNT(CampaignMagic);
    uint16 Version = 0;
    uint16 RecordCount = 0;
    if (!ReadU16(Bytes, Offset, Version) ||
        !ReadU16(Bytes, Offset, RecordCount))
    {
        OutError = TEXT("[CAMPAIGN_TRUNCATED] The campaign header is incomplete.");
        return false;
    }
    if (Version < FEchoesCampaignProgress::MinimumSupportedSchemaVersion ||
        Version > FEchoesCampaignProgress::SchemaVersion)
    {
        OutError = FString::Printf(
            TEXT("[CAMPAIGN_VERSION_UNSUPPORTED] Supported schemas are %u through %u; found %u."),
            FEchoesCampaignProgress::MinimumSupportedSchemaVersion,
            FEchoesCampaignProgress::SchemaVersion,
            Version);
        return false;
    }
    const int32 EncodedRecordSize = Version == 1
        ? LegacyRecordSize
        : RecordSize;
    if (RecordCount > FEchoesCampaignProgress::MaximumDecisionRecords ||
        Bytes.Num() !=
            HeaderSize + RecordCount * EncodedRecordSize + ChecksumSize)
    {
        OutError = TEXT("[CAMPAIGN_LENGTH_INVALID] The ledger record count is inconsistent with its length.");
        return false;
    }

    FEchoesCampaignProgress Candidate;
    Candidate.Decisions.Reserve(RecordCount);
    for (uint16 RecordIndex = 0; RecordIndex < RecordCount; ++RecordIndex)
    {
        uint8 Mission = 0;
        uint8 Choice = 0;
        uint8 Resolution = 0;
        FEchoesCampaignDecisionRecord Record;
        if (!ReadU8(Bytes, Offset, Mission) ||
            !ReadU8(Bytes, Offset, Choice) ||
            !ReadU8(Bytes, Offset, Record.AvailableWellChoices) ||
            !ReadU8(Bytes, Offset, Record.VerifiedFacts))
        {
            OutError = TEXT("[CAMPAIGN_TRUNCATED] A campaign record is incomplete.");
            return false;
        }
        if (Version >= 2 &&
            (!ReadU8(Bytes, Offset, Resolution) ||
             !ReadU8(
                 Bytes,
                 Offset,
                 Record.AvailableFinalResolutions) ||
             !ReadU8(Bytes, Offset, Record.FinalPlanKey)))
        {
            OutError = TEXT("[CAMPAIGN_TRUNCATED] A campaign final-resolution record is incomplete.");
            return false;
        }
        if (!ReadU32(Bytes, Offset, Record.SimulationSnapshotVersion) ||
            !ReadU64(Bytes, Offset, Record.CompletionTick) ||
            !ReadU64(Bytes, Offset, Record.FinalStateChecksum))
        {
            OutError = TEXT("[CAMPAIGN_TRUNCATED] A campaign record is incomplete.");
            return false;
        }
        Record.Mission = static_cast<EEchoesCampaignMissionId>(Mission);
        Record.WellChoice =
            static_cast<echoes::sim::FutureWellChoice>(Choice);
        Record.FinalResolution =
            static_cast<EEchoesFinalResolution>(Resolution);
        FString AppendFeedback;
        const EEchoesCampaignCommitStatus Status =
            Candidate.AppendDecision(Record, AppendFeedback);
        if (Status != EEchoesCampaignCommitStatus::Added)
        {
            OutError = AppendFeedback;
            return false;
        }
    }
    OutProgress = MoveTemp(Candidate);
    return true;
}

bool FEchoesCampaignProgressStore::SaveAtomic(
    const FString& Path,
    const FEchoesCampaignProgress& Progress,
    FString& OutFeedback)
{
    TArray<uint8> Bytes;
    if (!Encode(Progress, Bytes, OutFeedback))
    {
        return false;
    }

    IFileManager& Files = IFileManager::Get();
    const FString Directory = FPaths::GetPath(Path);
    const FString TemporaryPath = Path + TEXT(".tmp");
    const FString BackupPath = Path + TEXT(".bak");
    if (!Files.MakeDirectory(*Directory, true))
    {
        OutFeedback = TEXT("[CAMPAIGN_DIRECTORY_FAILED] The campaign save directory could not be created.");
        return false;
    }
    Files.Delete(*TemporaryPath, false, true, true);
    if (!FFileHelper::SaveArrayToFile(Bytes, *TemporaryPath))
    {
        OutFeedback = TEXT("[CAMPAIGN_WRITE_FAILED] The temporary campaign ledger could not be written.");
        return false;
    }

    FEchoesCampaignProgress Verification;
    FString VerificationError;
    if (!TryLoadOne(TemporaryPath, Verification, VerificationError) ||
        Verification.Decisions != Progress.Decisions)
    {
        Files.Delete(*TemporaryPath, false, true, true);
        OutFeedback = FString::Printf(
            TEXT("[CAMPAIGN_VALIDATION_FAILED] %s"),
            VerificationError.IsEmpty()
                ? TEXT("The temporary ledger did not reproduce the source state.")
                : *VerificationError);
        return false;
    }

    const bool bHadPrimary = Files.FileExists(*Path);
    bool bRetainedValidPrimary = false;
    if (bHadPrimary)
    {
        FEchoesCampaignProgress PriorPrimary;
        FString PriorPrimaryError;
        if (TryLoadOne(Path, PriorPrimary, PriorPrimaryError))
        {
            Files.Delete(*BackupPath, false, true, true);
            if (!Files.Move(*BackupPath, *Path, true, true, true, true))
            {
                Files.Delete(*TemporaryPath, false, true, true);
                OutFeedback = TEXT("[CAMPAIGN_BACKUP_FAILED] The prior campaign ledger could not be retained.");
                return false;
            }
            bRetainedValidPrimary = true;
        }
    }
    if (!Files.Move(*Path, *TemporaryPath, true, true, true, true))
    {
        if (bRetainedValidPrimary && Files.FileExists(*BackupPath))
        {
            Files.Move(*Path, *BackupPath, true, true, true, true);
        }
        Files.Delete(*TemporaryPath, false, true, true);
        OutFeedback = TEXT("[CAMPAIGN_COMMIT_FAILED] The validated campaign ledger was not committed.");
        return false;
    }
    OutFeedback = FString::Printf(
        TEXT("CAMPAIGN LEDGER: %d decision%s committed."),
        Progress.Decisions.Num(),
        Progress.Decisions.Num() == 1 ? TEXT("") : TEXT("s"));
    return true;
}

bool FEchoesCampaignProgressStore::LoadWithBackup(
    const FString& Path,
    FEchoesCampaignProgress& OutProgress,
    FString& OutFeedback)
{
    IFileManager& Files = IFileManager::Get();
    const FString BackupPath = Path + TEXT(".bak");
    const bool bPrimaryExists = Files.FileExists(*Path);
    const bool bBackupExists = Files.FileExists(*BackupPath);
    if (!bPrimaryExists && !bBackupExists)
    {
        OutProgress = FEchoesCampaignProgress{};
        OutFeedback = TEXT("CAMPAIGN LEDGER: no prior campaign; a new record will begin on mission completion.");
        return true;
    }

    FEchoesCampaignProgress Candidate;
    FString PrimaryFailure;
    if (bPrimaryExists && TryLoadOne(Path, Candidate, PrimaryFailure))
    {
        OutProgress = MoveTemp(Candidate);
        OutFeedback = TEXT("CAMPAIGN LEDGER: primary record loaded.");
        return true;
    }

    FString BackupFailure;
    if (bBackupExists && TryLoadOne(BackupPath, Candidate, BackupFailure))
    {
        OutProgress = MoveTemp(Candidate);
        OutFeedback = TEXT("CAMPAIGN LEDGER: prior-generation backup recovered.");
        return true;
    }

    OutFeedback = FString::Printf(
        TEXT("[CAMPAIGN_NO_VALID_LEDGER] primary=%s; backup=%s"),
        bPrimaryExists ? *PrimaryFailure : TEXT("file unavailable"),
        bBackupExists ? *BackupFailure : TEXT("file unavailable"));
    return false;
}

bool FEchoesCampaignProgressStore::LoadGeneration(
    const FString& Path,
    FEchoesCampaignProgress& OutProgress,
    FString& OutFeedback)
{
    if (!IFileManager::Get().FileExists(*Path))
    {
        OutFeedback = TEXT("[CAMPAIGN_GENERATION_UNAVAILABLE] The requested campaign generation does not exist.");
        return false;
    }
    if (!TryLoadOne(Path, OutProgress, OutFeedback))
    {
        OutFeedback = FString::Printf(
            TEXT("[CAMPAIGN_GENERATION_INVALID] %s"),
            *OutFeedback);
        return false;
    }
    OutFeedback = FString::Printf(
        TEXT("CAMPAIGN LEDGER: validated generation loaded with %d decision%s."),
        OutProgress.Decisions.Num(),
        OutProgress.Decisions.Num() == 1 ? TEXT("") : TEXT("s"));
    return true;
}

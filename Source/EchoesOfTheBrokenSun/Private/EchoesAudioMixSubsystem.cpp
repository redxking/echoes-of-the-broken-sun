#include "EchoesAudioMixSubsystem.h"

#include "AudioDevice.h"
#include "EchoesGameUserSettings.h"
#include "EchoesOfTheBrokenSun.h"
#include "Engine/World.h"
#include "Sound/SoundSubmix.h"

namespace
{
constexpr TCHAR MasterSubmixName[] = TEXT("EchoesMasterSubmix");

[[nodiscard]] const TCHAR* CategorySubmixName(EEchoesAudioCategory Category)
{
    switch (Category)
    {
        case EEchoesAudioCategory::Music:
            return TEXT("EchoesMusicSubmix");
        case EEchoesAudioCategory::Dialogue:
            return TEXT("EchoesDialogueSubmix");
        case EEchoesAudioCategory::Interface:
            return TEXT("EchoesInterfaceSubmix");
        case EEchoesAudioCategory::Ambience:
            return TEXT("EchoesAmbienceSubmix");
        case EEchoesAudioCategory::Effects:
        default:
            return TEXT("EchoesEffectsSubmix");
    }
}
} // namespace

void UEchoesAudioMixSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    CurrentDuckingGain = 1.0f;
    bDialogueDuckingActive = false;
    BuildGraph();
    ApplyPlayerVolumes();

    UE_LOG(
        LogEchoes,
        Display,
        TEXT("[ECHOES_AUDIO_MIX_READY] categories=%d graphComplete=%s masterRouting=%s music=%.3f dialogue=%.3f interface=%.3f ambience=%.3f effects=%.3f reducedDynamicRange=%s runtimeAuthority=presentation finalMix=false"),
        EchoesAudioCategoryCount,
        HasCompleteGraph() ? TEXT("true") : TEXT("false"),
        HasMasterRouting() ? TEXT("true") : TEXT("false"),
        GetAppliedCategoryGain(EEchoesAudioCategory::Music),
        GetAppliedCategoryGain(EEchoesAudioCategory::Dialogue),
        GetAppliedCategoryGain(EEchoesAudioCategory::Interface),
        GetAppliedCategoryGain(EEchoesAudioCategory::Ambience),
        GetAppliedCategoryGain(EEchoesAudioCategory::Effects),
        bAppliedReducedDynamicRange ? TEXT("true") : TEXT("false"));
}

void UEchoesAudioMixSubsystem::Deinitialize()
{
    // Applying a submix volume makes the audio device load and root the
    // submix object. A rooted per-world submix outlives its world and trips
    // the editor's world-leak check, so teardown must unregister each submix
    // and drop any root the device left behind before the world is destroyed.
    UWorld* World = GetWorld();
    FAudioDevice* AudioDevice =
        World != nullptr ? World->GetAudioDeviceRaw() : nullptr;

    auto ReleaseSubmix = [AudioDevice](USoundSubmix* Submix)
    {
        if (Submix == nullptr)
        {
            return;
        }
        if (AudioDevice != nullptr)
        {
            AudioDevice->UnregisterSoundSubmix(Submix, false);
        }
        if (Submix->IsRooted())
        {
            Submix->RemoveFromRoot();
        }
    };

    for (const TObjectPtr<USoundSubmix>& Submix : CategorySubmixes)
    {
        ReleaseSubmix(Submix.Get());
    }
    ReleaseSubmix(MasterSubmix.Get());
    CategorySubmixes.Reset();
    MasterSubmix = nullptr;

    Super::Deinitialize();
}

void UEchoesAudioMixSubsystem::BuildGraph()
{
    // The graph is transient and subsystem-owned. It is rebuilt per world so a
    // stale submix from a torn-down world can never outlive its owner.
    MasterSubmix = NewObject<USoundSubmix>(this, MasterSubmixName, RF_Transient);
    if (MasterSubmix == nullptr)
    {
        return;
    }
    MasterSubmix->bAutoDisable = false;

    CategorySubmixes.Reset();
    CategorySubmixes.SetNum(EchoesAudioCategoryCount);
    for (const EEchoesAudioCategory Category : EchoesAudioCategories)
    {
        USoundSubmix* Submix = NewObject<USoundSubmix>(
            this,
            CategorySubmixName(Category),
            RF_Transient);
        if (Submix == nullptr)
        {
            continue;
        }
        Submix->bAutoDisable = false;
        Submix->ParentSubmix = MasterSubmix;
        MasterSubmix->ChildSubmixes.Add(Submix);
        CategorySubmixes[EchoesAudioMix::CategoryIndex(Category)] = Submix;
    }
}

void UEchoesAudioMixSubsystem::ApplyPlayerVolumes()
{
    const UEchoesGameUserSettings* Settings = UEchoesGameUserSettings::Get();
    if (Settings == nullptr)
    {
        return;
    }
    ApplyVolumes(
        Settings->GetAudioMixVolumes(),
        Settings->IsReducedDynamicRangeEnabled());
}

void UEchoesAudioMixSubsystem::ApplyVolumes(
    const FEchoesAudioMixVolumes& Volumes,
    const bool bReducedDynamicRange)
{
    AppliedVolumes = Volumes;
    bAppliedReducedDynamicRange = bReducedDynamicRange;

    UWorld* World = GetWorld();
    const bool bHasAudioDevice =
        World != nullptr && World->GetAudioDeviceRaw() != nullptr;

    for (const EEchoesAudioCategory Category : EchoesAudioCategories)
    {
        const int32 Index = EchoesAudioMix::CategoryIndex(Category);
        const float Gain = EchoesAudioMix::ResolveCategoryGain(
            Volumes,
            Category,
            bReducedDynamicRange);
        BaseAppliedGains[Index] = Gain;

        const float DuckMultiplier =
            EchoesAudioMix::IsCategoryDuckedByDialogue(Category)
                ? CurrentDuckingGain
                : 1.0f;
        AppliedGains[Index] = Gain * DuckMultiplier;

        USoundSubmix* Submix = CategorySubmixes.IsValidIndex(Index)
            ? CategorySubmixes[Index].Get()
            : nullptr;
        if (Submix != nullptr && bHasAudioDevice)
        {
            Submix->SetSubmixOutputVolume(World, AppliedGains[Index]);
        }
    }
}

float UEchoesAudioMixSubsystem::GetAppliedCategoryGain(
    const EEchoesAudioCategory Category) const
{
    const int32 Index = EchoesAudioMix::CategoryIndex(Category);
    if (Index < 0 || Index >= EchoesAudioCategoryCount)
    {
        return 0.0f;
    }
    return AppliedGains[Index];
}

USoundSubmix* UEchoesAudioMixSubsystem::GetCategorySubmix(
    const EEchoesAudioCategory Category) const
{
    const int32 Index = EchoesAudioMix::CategoryIndex(Category);
    return CategorySubmixes.IsValidIndex(Index)
        ? CategorySubmixes[Index].Get()
        : nullptr;
}

bool UEchoesAudioMixSubsystem::HasCompleteGraph() const
{
    if (MasterSubmix == nullptr ||
        CategorySubmixes.Num() != EchoesAudioCategoryCount)
    {
        return false;
    }
    for (const TObjectPtr<USoundSubmix>& Submix : CategorySubmixes)
    {
        if (Submix == nullptr)
        {
            return false;
        }
    }
    return true;
}

bool UEchoesAudioMixSubsystem::HasMasterRouting() const
{
    if (!HasCompleteGraph())
    {
        return false;
    }
    if (MasterSubmix->ChildSubmixes.Num() != EchoesAudioCategoryCount)
    {
        return false;
    }
    for (const TObjectPtr<USoundSubmix>& Submix : CategorySubmixes)
    {
        if (Submix->ParentSubmix != MasterSubmix ||
            !MasterSubmix->ChildSubmixes.Contains(Submix))
        {
            return false;
        }
    }
    return MasterSubmix->ParentSubmix == nullptr;
}

float UEchoesAudioMixSubsystem::GetAppliedGainSpread() const
{
    float Lowest = AppliedGains[0];
    float Highest = AppliedGains[0];
    for (int32 Index = 1; Index < EchoesAudioCategoryCount; ++Index)
    {
        Lowest = FMath::Min(Lowest, AppliedGains[Index]);
        Highest = FMath::Max(Highest, AppliedGains[Index]);
    }
    return Highest - Lowest;
}

void UEchoesAudioMixSubsystem::SetDialogueDuckingActive(const bool bActive)
{
    bDialogueDuckingActive = bActive;
}

void UEchoesAudioMixSubsystem::AdvanceDucking(const float DeltaSeconds)
{
    const float TargetGain = bDialogueDuckingActive
        ? EchoesAudioMix::DialogueDuckingGainMultiplier
        : 1.0f;

    if (FMath::IsNearlyEqual(CurrentDuckingGain, TargetGain, 0.0001f))
    {
        CurrentDuckingGain = TargetGain;
        RefreshSubmixVolumes();
        return;
    }

    if (CurrentDuckingGain > TargetGain)
    {
        // Attack phase (gain dropping toward -9 dB target)
        const float Rate = (1.0f - EchoesAudioMix::DialogueDuckingGainMultiplier) /
            EchoesAudioMix::DialogueDuckingAttackSeconds;
        CurrentDuckingGain = FMath::Max(TargetGain, CurrentDuckingGain - Rate * DeltaSeconds);
    }
    else
    {
        // Release phase (gain rising toward unity 1.0)
        const float Rate = (1.0f - EchoesAudioMix::DialogueDuckingGainMultiplier) /
            EchoesAudioMix::DialogueDuckingReleaseSeconds;
        CurrentDuckingGain = FMath::Min(TargetGain, CurrentDuckingGain + Rate * DeltaSeconds);
    }

    RefreshSubmixVolumes();
}

void UEchoesAudioMixSubsystem::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    AdvanceDucking(DeltaTime);
}

void UEchoesAudioMixSubsystem::RefreshSubmixVolumes()
{
    UWorld* World = GetWorld();
    const bool bHasAudioDevice =
        World != nullptr && World->GetAudioDeviceRaw() != nullptr;

    for (const EEchoesAudioCategory Category : EchoesAudioCategories)
    {
        const int32 Index = EchoesAudioMix::CategoryIndex(Category);
        const float DuckMultiplier =
            EchoesAudioMix::IsCategoryDuckedByDialogue(Category)
                ? CurrentDuckingGain
                : 1.0f;
        AppliedGains[Index] = BaseAppliedGains[Index] * DuckMultiplier;

        USoundSubmix* Submix = CategorySubmixes.IsValidIndex(Index)
            ? CategorySubmixes[Index].Get()
            : nullptr;
        if (Submix != nullptr && bHasAudioDevice)
        {
            Submix->SetSubmixOutputVolume(World, AppliedGains[Index]);
        }
    }
}

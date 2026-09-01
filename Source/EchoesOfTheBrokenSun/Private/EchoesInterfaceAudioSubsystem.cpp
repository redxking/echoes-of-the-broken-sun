#include "EchoesInterfaceAudioSubsystem.h"

#include "EchoesAudioMixSubsystem.h"
#include "EchoesGameUserSettings.h"
#include "EchoesOfTheBrokenSun.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundSubmix.h"

namespace
{
constexpr const TCHAR* InterfaceCuePaths[] = {
    TEXT("/Game/Audio/Generated/UI_Hover.UI_Hover"),
    TEXT("/Game/Audio/Generated/UI_Select.UI_Select"),
    TEXT("/Game/Audio/Generated/UI_Confirm.UI_Confirm"),
    TEXT("/Game/Audio/Generated/UI_Reject.UI_Reject"),
    TEXT("/Game/Audio/Generated/UI_MenuOpen.UI_MenuOpen"),
    TEXT("/Game/Audio/Generated/UI_MenuClose.UI_MenuClose"),
    TEXT("/Game/Audio/Generated/UI_BriefAdvance.UI_BriefAdvance"),
};
constexpr const TCHAR* AlertCuePaths[] = {
    TEXT("/Game/Audio/Generated/ALERT_UnderAttack.ALERT_UnderAttack"),
    TEXT("/Game/Audio/Generated/ALERT_StructureLost.ALERT_StructureLost"),
    TEXT(
        "/Game/Audio/Generated/ALERT_ProductionComplete.ALERT_ProductionComplete"),
    TEXT(
        "/Game/Audio/Generated/ALERT_ResearchComplete.ALERT_ResearchComplete"),
    TEXT("/Game/Audio/Generated/ALERT_CapacityLow.ALERT_CapacityLow"),
};

[[nodiscard]] FName InterfaceCueKey(const TCHAR* Path)
{
    return FName(Path);
}
}

void UEchoesInterfaceAudioSubsystem::Initialize(
    FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    Collection.InitializeDependency(UEchoesAudioMixSubsystem::StaticClass());

    for (const TCHAR* Path : InterfaceCuePaths)
    {
        if (USoundBase* Sound = LoadCue(Path))
        {
            LoadedCues.Add(InterfaceCueKey(Path), Sound);
        }
    }
    for (const TCHAR* Path : AlertCuePaths)
    {
        if (USoundBase* Sound = LoadCue(Path))
        {
            LoadedCues.Add(InterfaceCueKey(Path), Sound);
        }
    }

    UE_LOG(
        LogEchoes,
        Display,
        TEXT("[ECHOES_INTERFACE_AUDIO_READY] cues=%d authored=%s routing=%s interfaceCooldownMs=60 alertCooldownMs=4000 runtimeAuthority=presentation finalAudio=false"),
        GetLoadedCueCount(),
        HasAllAuthoredCues() ? TEXT("true") : TEXT("false"),
        HasInterfaceSubmixRouting() ? TEXT("true") : TEXT("false"));
}

void UEchoesInterfaceAudioSubsystem::Deinitialize()
{
    // The cues are standalone assets that outlive this world. Clearing the
    // per-world submix routing here is mandatory: a stale SoundSubmixObject
    // pins the submix -> subsystem -> world chain and trips the editor's
    // world-leak check on the next map load.
    for (TPair<FName, TObjectPtr<USoundBase>>& Pair : LoadedCues)
    {
        if (Pair.Value != nullptr)
        {
            Pair.Value->SoundSubmixObject = nullptr;
        }
    }
    LoadedCues.Reset();
    Super::Deinitialize();
}

USoundBase* UEchoesInterfaceAudioSubsystem::LoadCue(const TCHAR* Path)
{
    USoundBase* Sound = LoadObject<USoundBase>(nullptr, Path);
    if (Sound == nullptr)
    {
        return nullptr;
    }
    if (UWorld* World = GetWorld())
    {
        if (UEchoesAudioMixSubsystem* Mix =
                World->GetSubsystem<UEchoesAudioMixSubsystem>())
        {
            if (USoundSubmix* InterfaceSubmix =
                    Mix->GetCategorySubmix(EEchoesAudioCategory::Interface))
            {
                Sound->SoundSubmixObject = InterfaceSubmix;
            }
        }
    }
    return Sound;
}

USoundBase* UEchoesInterfaceAudioSubsystem::ResolveInterfaceCue(
    EEchoesInterfaceCue Cue) const
{
    const int32 Index = static_cast<int32>(Cue);
    if (Index < 0 || Index >= UE_ARRAY_COUNT(InterfaceCuePaths))
    {
        return nullptr;
    }
    const TObjectPtr<USoundBase>* Found =
        LoadedCues.Find(InterfaceCueKey(InterfaceCuePaths[Index]));
    return Found != nullptr ? Found->Get() : nullptr;
}

USoundBase* UEchoesInterfaceAudioSubsystem::ResolveAlertCue(
    EEchoesAlertCue Alert) const
{
    const int32 Index = static_cast<int32>(Alert);
    if (Index < 0 || Index >= UE_ARRAY_COUNT(AlertCuePaths))
    {
        return nullptr;
    }
    const TObjectPtr<USoundBase>* Found =
        LoadedCues.Find(InterfaceCueKey(AlertCuePaths[Index]));
    return Found != nullptr ? Found->Get() : nullptr;
}

bool UEchoesInterfaceAudioSubsystem::IsInterfaceAudible() const
{
    const UEchoesGameUserSettings* Settings = UEchoesGameUserSettings::Get();
    if (Settings == nullptr)
    {
        return false;
    }
    return Settings->GetInterfaceVolume() > 0.0f &&
        Settings->GetMasterVolume() > 0.0f;
}

bool UEchoesInterfaceAudioSubsystem::ReserveInterfaceCue(
    EEchoesInterfaceCue Cue,
    double Seconds)
{
    if (!FMath::IsFinite(Seconds) || !IsInterfaceAudible())
    {
        return false;
    }
    const int32 Index = static_cast<int32>(Cue);
    if (Index < 0 || Index >= UE_ARRAY_COUNT(LastInterfaceSeconds))
    {
        return false;
    }
    double& Last = LastInterfaceSeconds[Index];
    if (Seconds < Last || Seconds - Last < GetInterfaceCooldownSeconds())
    {
        return false;
    }
    Last = Seconds;
    return true;
}

bool UEchoesInterfaceAudioSubsystem::ReserveAlert(
    EEchoesAlertCue Alert,
    double Seconds)
{
    if (!FMath::IsFinite(Seconds) || !IsInterfaceAudible())
    {
        return false;
    }
    const int32 Index = static_cast<int32>(Alert);
    if (Index < 0 || Index >= UE_ARRAY_COUNT(LastAlertSeconds))
    {
        return false;
    }
    double& Last = LastAlertSeconds[Index];
    if (Seconds < Last || Seconds - Last < GetAlertCooldownSeconds())
    {
        return false;
    }
    Last = Seconds;
    return true;
}

bool UEchoesInterfaceAudioSubsystem::PlayInterfaceCue(EEchoesInterfaceCue Cue)
{
    UWorld* World = GetWorld();
    USoundBase* Sound = ResolveInterfaceCue(Cue);
    if (World == nullptr || Sound == nullptr ||
        !ReserveInterfaceCue(Cue, World->GetRealTimeSeconds()))
    {
        return false;
    }
    UGameplayStatics::PlaySound2D(World, Sound);
    ++InterfacePlayCount;
    return true;
}

bool UEchoesInterfaceAudioSubsystem::PlayAlert(EEchoesAlertCue Alert)
{
    UWorld* World = GetWorld();
    USoundBase* Sound = ResolveAlertCue(Alert);
    if (World == nullptr || Sound == nullptr ||
        !ReserveAlert(Alert, World->GetRealTimeSeconds()))
    {
        return false;
    }
    UGameplayStatics::PlaySound2D(World, Sound);
    ++AlertPlayCount;
    UE_LOG(
        LogEchoes,
        Verbose,
        TEXT("[ECHOES_ALERT_AUDIO] alert=%d played=true"),
        static_cast<int32>(Alert));
    return true;
}

bool UEchoesInterfaceAudioSubsystem::HasAllAuthoredCues() const
{
    return GetLoadedCueCount() ==
        UE_ARRAY_COUNT(InterfaceCuePaths) + UE_ARRAY_COUNT(AlertCuePaths);
}

int32 UEchoesInterfaceAudioSubsystem::GetLoadedCueCount() const
{
    return LoadedCues.Num();
}

bool UEchoesInterfaceAudioSubsystem::HasInterfaceSubmixRouting() const
{
    UWorld* World = GetWorld();
    UEchoesAudioMixSubsystem* Mix =
        World != nullptr ? World->GetSubsystem<UEchoesAudioMixSubsystem>()
                         : nullptr;
    USoundSubmix* InterfaceSubmix =
        Mix != nullptr
            ? Mix->GetCategorySubmix(EEchoesAudioCategory::Interface)
            : nullptr;
    if (InterfaceSubmix == nullptr || LoadedCues.IsEmpty())
    {
        return false;
    }
    for (const TPair<FName, TObjectPtr<USoundBase>>& Pair : LoadedCues)
    {
        if (Pair.Value == nullptr ||
            Pair.Value->SoundSubmixObject != InterfaceSubmix)
        {
            return false;
        }
    }
    return true;
}

#if WITH_DEV_AUTOMATION_TESTS
bool UEchoesInterfaceAudioSubsystem::ReserveInterfaceCueForTest(
    EEchoesInterfaceCue Cue,
    double Seconds)
{
    return ReserveInterfaceCue(Cue, Seconds);
}

bool UEchoesInterfaceAudioSubsystem::ReserveAlertForTest(
    EEchoesAlertCue Alert,
    double Seconds)
{
    return ReserveAlert(Alert, Seconds);
}

void UEchoesInterfaceAudioSubsystem::ResetRateLimitsForTest()
{
    for (double& Last : LastInterfaceSeconds)
    {
        Last = -1000.0;
    }
    for (double& Last : LastAlertSeconds)
    {
        Last = -1000.0;
    }
    InterfacePlayCount = 0;
    AlertPlayCount = 0;
}
#endif

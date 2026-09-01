#include "EchoesAmbienceSubsystem.h"

#include "Components/AudioComponent.h"
#include "EchoesAudioMixSubsystem.h"
#include "EchoesOfTheBrokenSun.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundSubmix.h"

namespace
{
constexpr TCHAR GlassScarCuePath[] =
    TEXT("/Game/Audio/Generated/AMB_GlassScar.AMB_GlassScar");
constexpr TCHAR LumeReachCuePath[] =
    TEXT("/Game/Audio/Generated/AMB_LumeReach.AMB_LumeReach");
constexpr TCHAR ArkCityCuePath[] =
    TEXT("/Game/Audio/Generated/AMB_ArkCity.AMB_ArkCity");
constexpr TCHAR CrownfallCuePath[] =
    TEXT("/Game/Audio/Generated/AMB_Crownfall.AMB_Crownfall");
constexpr TCHAR FutureWellCuePath[] =
    TEXT("/Game/Audio/Generated/AMB_FutureWell.AMB_FutureWell");

constexpr const TCHAR* AllAmbienceCuePaths[] = {
    GlassScarCuePath,
    LumeReachCuePath,
    ArkCityCuePath,
    CrownfallCuePath,
    FutureWellCuePath,
};

[[nodiscard]] FName AmbienceCueKey(const TCHAR* Path)
{
    return FName(Path);
}
}

void UEchoesAmbienceSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    Collection.InitializeDependency(UEchoesAudioMixSubsystem::StaticClass());

    for (const TCHAR* Path : AllAmbienceCuePaths)
    {
        USoundBase* Sound = LoadCue(Path);
        if (Sound != nullptr)
        {
            LoadedCues.Add(AmbienceCueKey(Path), Sound);
        }
    }

    UE_LOG(
        LogEchoes,
        Display,
        TEXT("[ECHOES_AMBIENCE_READY] cues=%d authored=%s routing=%s crossfadeSeconds=%.2f runtimeAuthority=presentation finalAudio=false"),
        GetLoadedCueCount(),
        HasAllAuthoredCues() ? TEXT("true") : TEXT("false"),
        HasAmbienceSubmixRouting() ? TEXT("true") : TEXT("false"),
        GetCrossfadeSeconds());
}

void UEchoesAmbienceSubsystem::Deinitialize()
{
    StopAllAmbience();
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

USoundBase* UEchoesAmbienceSubsystem::LoadCue(const TCHAR* Path)
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
            if (USoundSubmix* AmbienceSubmix =
                    Mix->GetCategorySubmix(EEchoesAudioCategory::Ambience))
            {
                Sound->SoundSubmixObject = AmbienceSubmix;
            }
        }
    }
    return Sound;
}

void UEchoesAmbienceSubsystem::StopAllAmbience()
{
    auto StopComponent = [](TObjectPtr<UAudioComponent>& Component)
    {
        if (Component != nullptr)
        {
            Component->Stop();
            Component = nullptr;
        }
    };
    StopComponent(BedComponent);
    StopComponent(WellComponent);
    CurrentBed = EEchoesAmbienceBed::None;
    bWellActive = false;
}

USoundBase* UEchoesAmbienceSubsystem::ResolveBedCue(
    EEchoesAmbienceBed Bed) const
{
    const TCHAR* Path = nullptr;
    switch (Bed)
    {
        case EEchoesAmbienceBed::None:
            return nullptr;
        case EEchoesAmbienceBed::GlassScar:
            Path = GlassScarCuePath;
            break;
        case EEchoesAmbienceBed::LumeReach:
            Path = LumeReachCuePath;
            break;
        case EEchoesAmbienceBed::ArkCity:
            Path = ArkCityCuePath;
            break;
        case EEchoesAmbienceBed::Crownfall:
            Path = CrownfallCuePath;
            break;
    }
    const TObjectPtr<USoundBase>* Found = LoadedCues.Find(AmbienceCueKey(Path));
    return Found != nullptr ? Found->Get() : nullptr;
}

void UEchoesAmbienceSubsystem::SetAmbienceBed(EEchoesAmbienceBed Bed)
{
    if (Bed == CurrentBed)
    {
        return;
    }
    if (BedComponent != nullptr)
    {
        BedComponent->FadeOut(GetCrossfadeSeconds(), 0.0f);
        BedComponent = nullptr;
    }
    CurrentBed = Bed;

    UWorld* World = GetWorld();
    USoundBase* Cue = ResolveBedCue(Bed);
    if (World != nullptr && Cue != nullptr)
    {
        BedComponent = UGameplayStatics::SpawnSound2D(
            World,
            Cue,
            1.0f,
            1.0f,
            0.0f,
            nullptr,
            /*bPersistAcrossLevelTransition=*/false,
            /*bAutoDestroy=*/false);
        if (BedComponent != nullptr)
        {
            BedComponent->FadeIn(GetCrossfadeSeconds(), 1.0f);
        }
    }
    UE_LOG(
        LogEchoes,
        Verbose,
        TEXT("[ECHOES_AMBIENCE_BED] bed=%d playing=%s"),
        static_cast<int32>(Bed),
        BedComponent != nullptr ? TEXT("true") : TEXT("false"));
}

void UEchoesAmbienceSubsystem::SetWellProximity(bool bNearWell)
{
    if (bNearWell == bWellActive)
    {
        return;
    }
    bWellActive = bNearWell;
    if (bNearWell)
    {
        const TObjectPtr<USoundBase>* Found =
            LoadedCues.Find(AmbienceCueKey(FutureWellCuePath));
        USoundBase* Cue = Found != nullptr ? Found->Get() : nullptr;
        UWorld* World = GetWorld();
        if (World != nullptr && Cue != nullptr && WellComponent == nullptr)
        {
            WellComponent = UGameplayStatics::SpawnSound2D(
                World,
                Cue,
                1.0f,
                1.0f,
                0.0f,
                nullptr,
                /*bPersistAcrossLevelTransition=*/false,
                /*bAutoDestroy=*/false);
            if (WellComponent != nullptr)
            {
                WellComponent->FadeIn(GetCrossfadeSeconds(), 1.0f);
            }
        }
    }
    else if (WellComponent != nullptr)
    {
        WellComponent->FadeOut(GetCrossfadeSeconds(), 0.0f);
        WellComponent = nullptr;
    }
}

bool UEchoesAmbienceSubsystem::HasAllAuthoredCues() const
{
    return GetLoadedCueCount() == UE_ARRAY_COUNT(AllAmbienceCuePaths);
}

int32 UEchoesAmbienceSubsystem::GetLoadedCueCount() const
{
    return LoadedCues.Num();
}

bool UEchoesAmbienceSubsystem::HasAmbienceSubmixRouting() const
{
    UWorld* World = GetWorld();
    UEchoesAudioMixSubsystem* Mix =
        World != nullptr ? World->GetSubsystem<UEchoesAudioMixSubsystem>()
                         : nullptr;
    USoundSubmix* AmbienceSubmix =
        Mix != nullptr
            ? Mix->GetCategorySubmix(EEchoesAudioCategory::Ambience)
            : nullptr;
    if (AmbienceSubmix == nullptr || LoadedCues.IsEmpty())
    {
        return false;
    }
    for (const TPair<FName, TObjectPtr<USoundBase>>& Pair : LoadedCues)
    {
        if (Pair.Value == nullptr ||
            Pair.Value->SoundSubmixObject != AmbienceSubmix)
        {
            return false;
        }
    }
    return true;
}

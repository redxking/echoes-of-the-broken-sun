#include "EchoesMusicSubsystem.h"

#include "AudioDevice.h"
#include "Components/AudioComponent.h"
#include "EchoesAudioMixSubsystem.h"
#include "EchoesOfTheBrokenSun.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundSubmix.h"

namespace
{
constexpr TCHAR TitleCuePath[] =
    TEXT("/Game/Audio/Generated/MUS_Title.MUS_Title");
constexpr TCHAR MeridianCuePath[] =
    TEXT("/Game/Audio/Generated/MUS_Meridian.MUS_Meridian");
constexpr TCHAR KharuunCuePath[] =
    TEXT("/Game/Audio/Generated/MUS_Kharuun.MUS_Kharuun");
constexpr TCHAR ChoirCuePath[] =
    TEXT("/Game/Audio/Generated/MUS_Choir.MUS_Choir");
constexpr TCHAR ActICuePath[] =
    TEXT("/Game/Audio/Generated/MUS_ActI.MUS_ActI");
constexpr TCHAR ActIICuePath[] =
    TEXT("/Game/Audio/Generated/MUS_ActII.MUS_ActII");
constexpr TCHAR ActIIICuePath[] =
    TEXT("/Game/Audio/Generated/MUS_ActIII.MUS_ActIII");
constexpr TCHAR TensionCuePath[] =
    TEXT("/Game/Audio/Generated/MUS_TensionLayer.MUS_TensionLayer");
constexpr TCHAR CombatCuePath[] =
    TEXT("/Game/Audio/Generated/MUS_CombatLayer.MUS_CombatLayer");
constexpr TCHAR VictoryCuePath[] =
    TEXT("/Game/Audio/Generated/MUS_Victory.MUS_Victory");
constexpr TCHAR DefeatCuePath[] =
    TEXT("/Game/Audio/Generated/MUS_Defeat.MUS_Defeat");
constexpr TCHAR EndingRestorationCuePath[] = TEXT(
    "/Game/Audio/Generated/MUS_EndingRestoration.MUS_EndingRestoration");
constexpr TCHAR EndingStabilizationCuePath[] = TEXT(
    "/Game/Audio/Generated/MUS_EndingStabilization.MUS_EndingStabilization");
constexpr TCHAR EndingExtinguishmentCuePath[] = TEXT(
    "/Game/Audio/Generated/MUS_EndingExtinguishment.MUS_EndingExtinguishment");
constexpr TCHAR EndingOpenEvolutionCuePath[] = TEXT(
    "/Game/Audio/Generated/MUS_EndingOpenEvolution.MUS_EndingOpenEvolution");

constexpr const TCHAR* PairingTags[] = {
    TEXT("MM"), TEXT("MK"), TEXT("MC"), TEXT("KK"), TEXT("KC"), TEXT("CC"),
};
constexpr TCHAR BriefUnderscorePath[] =
    TEXT("/Game/Audio/Generated/MUS_BriefUnderscore.MUS_BriefUnderscore");
constexpr TCHAR ResultsUnderscorePath[] =
    TEXT("/Game/Audio/Generated/MUS_ResultsUnderscore.MUS_ResultsUnderscore");

[[nodiscard]] FString PairingCuePath(const TCHAR* Kind, const FString& Tag)
{
    return FString::Printf(
        TEXT("/Game/Audio/Generated/MUS_%s%s.MUS_%s%s"),
        Kind,
        *Tag,
        Kind,
        *Tag);
}

[[nodiscard]] TCHAR FactionPairingLetter(echoes::sim::Faction Faction)
{
    switch (Faction)
    {
        case echoes::sim::Faction::MeridianCompact: return TEXT('M');
        case echoes::sim::Faction::KharuunAssemblies: return TEXT('K');
        case echoes::sim::Faction::HollowChoir: return TEXT('C');
    }
    return TEXT('M');
}

[[nodiscard]] int32 FactionPairingRank(echoes::sim::Faction Faction)
{
    switch (Faction)
    {
        case echoes::sim::Faction::MeridianCompact: return 0;
        case echoes::sim::Faction::KharuunAssemblies: return 1;
        case echoes::sim::Faction::HollowChoir: return 2;
    }
    return 0;
}

constexpr const TCHAR* AllMusicCuePaths[] = {
    TitleCuePath,          MeridianCuePath,
    KharuunCuePath,        ChoirCuePath,
    ActICuePath,           ActIICuePath,
    ActIIICuePath,         TensionCuePath,
    CombatCuePath,         VictoryCuePath,
    DefeatCuePath,         EndingRestorationCuePath,
    EndingStabilizationCuePath, EndingExtinguishmentCuePath,
    EndingOpenEvolutionCuePath,
};

[[nodiscard]] FName MusicCueKey(const TCHAR* Path)
{
    return FName(Path);
}
}

void UEchoesMusicSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    // The mix subsystem must exist first so cue routing can bind to the music
    // category submix at load time.
    Collection.InitializeDependency(UEchoesAudioMixSubsystem::StaticClass());

    for (const TCHAR* Path : AllMusicCuePaths)
    {
        USoundBase* Sound = LoadCue(Path);
        if (Sound != nullptr)
        {
            LoadedCues.Add(MusicCueKey(Path), Sound);
        }
    }
    for (const TCHAR* Tag : PairingTags)
    {
        for (const TCHAR* Kind : {TEXT("Tension"), TEXT("Combat")})
        {
            const FString Path = PairingCuePath(Kind, Tag);
            if (USoundBase* Sound = LoadCue(*Path))
            {
                LoadedCues.Add(FName(*Path), Sound);
            }
        }
    }
    for (const TCHAR* Path : {BriefUnderscorePath, ResultsUnderscorePath})
    {
        if (USoundBase* Sound = LoadCue(Path))
        {
            LoadedCues.Add(MusicCueKey(Path), Sound);
        }
    }

    UE_LOG(
        LogEchoes,
        Display,
        TEXT("[ECHOES_MUSIC_READY] cues=%d authored=%s routing=%s crossfadeSeconds=%.2f layerFadeSeconds=%.2f runtimeAuthority=presentation finalAudio=false"),
        GetLoadedCueCount(),
        HasAllAuthoredCues() ? TEXT("true") : TEXT("false"),
        HasMusicSubmixRouting() ? TEXT("true") : TEXT("false"),
        GetCrossfadeSeconds(),
        GetLayerFadeSeconds());
}

void UEchoesMusicSubsystem::Deinitialize()
{
    StopAllMusic();
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

USoundBase* UEchoesMusicSubsystem::LoadCue(const TCHAR* Path)
{
    USoundBase* Sound = LoadObject<USoundBase>(nullptr, Path);
    if (Sound == nullptr)
    {
        return nullptr;
    }
    // Route the cue's base submix into the music category so the player's
    // music volume owns its output. This is a transient runtime assignment.
    if (UWorld* World = GetWorld())
    {
        if (UEchoesAudioMixSubsystem* Mix =
                World->GetSubsystem<UEchoesAudioMixSubsystem>())
        {
            if (USoundSubmix* MusicSubmix =
                    Mix->GetCategorySubmix(EEchoesAudioCategory::Music))
            {
                Sound->SoundSubmixObject = MusicSubmix;
            }
        }
    }
    return Sound;
}

void UEchoesMusicSubsystem::StopAllMusic()
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
    StopComponent(TensionComponent);
    StopComponent(CombatComponent);
    CurrentContext = EEchoesMusicContext::Silent;
    bTensionActive = false;
    bCombatActive = false;
}

USoundBase* UEchoesMusicSubsystem::ResolveBedCue(
    EEchoesMusicContext Context,
    echoes::sim::Faction Faction,
    int32 ActIndex) const
{
    const TCHAR* Path = nullptr;
    switch (Context)
    {
        case EEchoesMusicContext::Silent:
            return nullptr;
        case EEchoesMusicContext::Title:
            Path = TitleCuePath;
            break;
        case EEchoesMusicContext::FactionTheme:
            switch (Faction)
            {
                case echoes::sim::Faction::MeridianCompact:
                    Path = MeridianCuePath;
                    break;
                case echoes::sim::Faction::KharuunAssemblies:
                    Path = KharuunCuePath;
                    break;
                case echoes::sim::Faction::HollowChoir:
                    Path = ChoirCuePath;
                    break;
            }
            break;
        case EEchoesMusicContext::ActBed:
            Path = ActIndex <= 1 ? ActICuePath
                 : ActIndex == 2 ? ActIICuePath
                                 : ActIIICuePath;
            break;
    }
    if (Path == nullptr)
    {
        return nullptr;
    }
    const TObjectPtr<USoundBase>* Found = LoadedCues.Find(MusicCueKey(Path));
    return Found != nullptr ? Found->Get() : nullptr;
}

USoundBase* UEchoesMusicSubsystem::ResolveStingerCue(
    EEchoesMusicStinger Stinger) const
{
    const TCHAR* Path = nullptr;
    switch (Stinger)
    {
        case EEchoesMusicStinger::Victory:
            Path = VictoryCuePath;
            break;
        case EEchoesMusicStinger::Defeat:
            Path = DefeatCuePath;
            break;
        case EEchoesMusicStinger::EndingRestoration:
            Path = EndingRestorationCuePath;
            break;
        case EEchoesMusicStinger::EndingStabilization:
            Path = EndingStabilizationCuePath;
            break;
        case EEchoesMusicStinger::EndingExtinguishment:
            Path = EndingExtinguishmentCuePath;
            break;
        case EEchoesMusicStinger::EndingOpenEvolution:
            Path = EndingOpenEvolutionCuePath;
            break;
    }
    const TObjectPtr<USoundBase>* Found = LoadedCues.Find(MusicCueKey(Path));
    return Found != nullptr ? Found->Get() : nullptr;
}

UAudioComponent* UEchoesMusicSubsystem::StartLoopingBed(
    USoundBase* Sound,
    float FadeSeconds)
{
    UWorld* World = GetWorld();
    if (World == nullptr || Sound == nullptr)
    {
        return nullptr;
    }
    UAudioComponent* Component = UGameplayStatics::SpawnSound2D(
        World,
        Sound,
        1.0f,
        1.0f,
        0.0f,
        nullptr,
        /*bPersistAcrossLevelTransition=*/false,
        /*bAutoDestroy=*/false);
    if (Component != nullptr && FadeSeconds > 0.0f)
    {
        Component->FadeIn(FadeSeconds, 1.0f);
    }
    return Component;
}

void UEchoesMusicSubsystem::SetMusicContext(
    EEchoesMusicContext Context,
    echoes::sim::Faction Faction,
    int32 ActIndex)
{
    const int32 ClampedAct = FMath::Clamp(ActIndex, 1, 3);
    const bool bUnchanged =
        Context == CurrentContext &&
        (Context != EEchoesMusicContext::FactionTheme ||
         Faction == CurrentFaction) &&
        (Context != EEchoesMusicContext::ActBed || ClampedAct == CurrentAct);
    if (bUnchanged)
    {
        return;
    }

    // Crossfade: the outgoing bed fades over the same window the incoming bed
    // fades in, so a context change is never a hard cut.
    if (BedComponent != nullptr)
    {
        BedComponent->FadeOut(GetCrossfadeSeconds(), 0.0f);
        BedComponent = nullptr;
    }

    CurrentContext = Context;
    CurrentFaction = Faction;
    CurrentAct = ClampedAct;

    USoundBase* Bed = ResolveBedCue(Context, Faction, ClampedAct);
    if (Bed != nullptr)
    {
        BedComponent = StartLoopingBed(Bed, GetCrossfadeSeconds());
    }
    UE_LOG(
        LogEchoes,
        Verbose,
        TEXT("[ECHOES_MUSIC_CONTEXT] context=%d faction=%d act=%d playing=%s"),
        static_cast<int32>(Context),
        static_cast<int32>(Faction),
        ClampedAct,
        BedComponent != nullptr ? TEXT("true") : TEXT("false"));
}

USoundBase* UEchoesMusicSubsystem::FindCue(const TCHAR* Path) const
{
    const TObjectPtr<USoundBase>* Found = LoadedCues.Find(FName(Path));
    return Found != nullptr ? Found->Get() : nullptr;
}

void UEchoesMusicSubsystem::SetThreatContext(
    echoes::sim::Faction Local,
    echoes::sim::Faction Opponent)
{
    const bool bLocalFirst =
        FactionPairingRank(Local) <= FactionPairingRank(Opponent);
    const TCHAR First =
        FactionPairingLetter(bLocalFirst ? Local : Opponent);
    const TCHAR Second =
        FactionPairingLetter(bLocalFirst ? Opponent : Local);
    const FString NewTag = FString::Printf(TEXT("%c%c"), First, Second);
    if (bThreatContextSet && NewTag == ThreatPairingTag)
    {
        return;
    }
    ThreatPairingTag = NewTag;
    bThreatContextSet = true;
    // A live layer restarts on its new material through the ordinary fades.
    const bool bTensionWas = bTensionActive;
    const bool bCombatWas = bCombatActive;
    SetThreatLayers(false, false);
    SetThreatLayers(bTensionWas, bCombatWas);
}

void UEchoesMusicSubsystem::ClearThreatContext()
{
    if (!bThreatContextSet)
    {
        return;
    }
    bThreatContextSet = false;
    ThreatPairingTag.Reset();
    const bool bTensionWas = bTensionActive;
    const bool bCombatWas = bCombatActive;
    SetThreatLayers(false, false);
    SetThreatLayers(bTensionWas, bCombatWas);
}

USoundBase* UEchoesMusicSubsystem::ResolveTensionCue() const
{
    if (bThreatContextSet)
    {
        if (USoundBase* Paired =
                FindCue(*PairingCuePath(TEXT("Tension"), ThreatPairingTag)))
        {
            return Paired;
        }
    }
    return FindCue(TensionCuePath);
}

USoundBase* UEchoesMusicSubsystem::ResolveCombatCue() const
{
    if (bThreatContextSet)
    {
        if (USoundBase* Paired =
                FindCue(*PairingCuePath(TEXT("Combat"), ThreatPairingTag)))
        {
            return Paired;
        }
    }
    return FindCue(CombatCuePath);
}

void UEchoesMusicSubsystem::SetThreatLayers(bool bTension, bool bCombat)
{
    auto UpdateLayer = [this](
                           TObjectPtr<UAudioComponent>& Component,
                           bool bWanted,
                           bool& bActive,
                           USoundBase* Sound)
    {
        if (bWanted == bActive)
        {
            return;
        }
        bActive = bWanted;
        if (bWanted)
        {
            if (Component == nullptr)
            {
                Component = StartLoopingBed(Sound, GetLayerFadeSeconds());
            }
            else
            {
                Component->FadeIn(GetLayerFadeSeconds(), 1.0f);
            }
        }
        else if (Component != nullptr)
        {
            Component->FadeOut(GetLayerFadeSeconds(), 0.0f);
            Component = nullptr;
        }
    };
    UpdateLayer(
        TensionComponent, bTension, bTensionActive, ResolveTensionCue());
    UpdateLayer(CombatComponent, bCombat, bCombatActive, ResolveCombatCue());
}

bool UEchoesMusicSubsystem::PlayStinger(EEchoesMusicStinger Stinger)
{
    UWorld* World = GetWorld();
    USoundBase* Sound = ResolveStingerCue(Stinger);
    if (World == nullptr || Sound == nullptr)
    {
        return false;
    }
    UGameplayStatics::PlaySound2D(World, Sound);
    ++StingerPlayCount;
    return true;
}

bool UEchoesMusicSubsystem::HasAllAuthoredCues() const
{
    return GetLoadedCueCount() ==
        UE_ARRAY_COUNT(AllMusicCuePaths) + UE_ARRAY_COUNT(PairingTags) * 2 + 2;
}

int32 UEchoesMusicSubsystem::GetLoadedCueCount() const
{
    return LoadedCues.Num();
}

bool UEchoesMusicSubsystem::HasMusicSubmixRouting() const
{
    UWorld* World = GetWorld();
    UEchoesAudioMixSubsystem* Mix =
        World != nullptr ? World->GetSubsystem<UEchoesAudioMixSubsystem>()
                         : nullptr;
    USoundSubmix* MusicSubmix =
        Mix != nullptr ? Mix->GetCategorySubmix(EEchoesAudioCategory::Music)
                       : nullptr;
    if (MusicSubmix == nullptr || LoadedCues.IsEmpty())
    {
        return false;
    }
    for (const TPair<FName, TObjectPtr<USoundBase>>& Pair : LoadedCues)
    {
        if (Pair.Value == nullptr ||
            Pair.Value->SoundSubmixObject != MusicSubmix)
        {
            return false;
        }
    }
    return true;
}

#include "EchoesPresentationAudioSubsystem.h"

#include "EchoesGameUserSettings.h"
#include "EchoesOfTheBrokenSun.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundAttenuation.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundConcurrency.h"

namespace
{
constexpr TCHAR CommandCuePath[] =
    TEXT("/Game/Audio/Generated/SFX_CommandConfirm.SFX_CommandConfirm");
constexpr TCHAR MeridianDestructionCuePath[] =
    TEXT("/Game/Audio/Generated/SFX_DestructionMeridian.SFX_DestructionMeridian");
constexpr TCHAR KharuunDestructionCuePath[] =
    TEXT("/Game/Audio/Generated/SFX_DestructionKharuun.SFX_DestructionKharuun");
constexpr TCHAR ChoirDestructionCuePath[] =
    TEXT("/Game/Audio/Generated/SFX_DestructionChoir.SFX_DestructionChoir");
constexpr float MinimumAudibleEffectsVolume = 0.005f;

[[nodiscard]] const TCHAR* CueStableName(EEchoesPresentationAudioCue Cue)
{
    switch (Cue)
    {
        case EEchoesPresentationAudioCue::DestructionMeridian:
            return TEXT("destruction_meridian");
        case EEchoesPresentationAudioCue::DestructionKharuun:
            return TEXT("destruction_kharuun");
        case EEchoesPresentationAudioCue::DestructionChoir:
            return TEXT("destruction_choir");
        case EEchoesPresentationAudioCue::CommandConfirm:
        default:
            return TEXT("command_confirm");
    }
}
}

void UEchoesPresentationAudioSubsystem::Initialize(
    FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    CommandConfirmSound = LoadObject<USoundBase>(nullptr, CommandCuePath);
    MeridianDestructionSound =
        LoadObject<USoundBase>(nullptr, MeridianDestructionCuePath);
    KharuunDestructionSound =
        LoadObject<USoundBase>(nullptr, KharuunDestructionCuePath);
    ChoirDestructionSound =
        LoadObject<USoundBase>(nullptr, ChoirDestructionCuePath);

    // ReserveCue owns the 80/140 ms windows. Keep engine retriggering at zero
    // so empty groups cannot outlive these transient policy objects.
    CommandConcurrency =
        NewObject<USoundConcurrency>(this, NAME_None, RF_Transient);
    if (CommandConcurrency != nullptr)
    {
        FSoundConcurrencySettings& Concurrency =
            CommandConcurrency->Concurrency;
        Concurrency.MaxCount = GetCommandMaxConcurrentVoices();
        Concurrency.bLimitToOwner = false;
        Concurrency.ResolutionRule =
            EMaxConcurrentResolutionRule::PreventNew;
        Concurrency.RetriggerTime = 0.0f;
    }

    DestructionConcurrency =
        NewObject<USoundConcurrency>(this, NAME_None, RF_Transient);
    if (DestructionConcurrency != nullptr)
    {
        FSoundConcurrencySettings& Concurrency =
            DestructionConcurrency->Concurrency;
        Concurrency.MaxCount = GetDestructionMaxConcurrentVoices();
        Concurrency.bLimitToOwner = false;
        Concurrency.ResolutionRule =
            EMaxConcurrentResolutionRule::StopFarthestThenOldest;
        Concurrency.RetriggerTime = 0.0f;
        Concurrency.VoiceStealReleaseTime = 0.05f;
    }

    DestructionAttenuation = NewObject<USoundAttenuation>(this);
    if (DestructionAttenuation != nullptr)
    {
        FSoundAttenuationSettings& Attenuation =
            DestructionAttenuation->Attenuation;
        Attenuation.bAttenuate = true;
        Attenuation.bSpatialize = true;
        Attenuation.DistanceAlgorithm = EAttenuationDistanceModel::Linear;
        Attenuation.AttenuationShape = EAttenuationShape::Sphere;
        Attenuation.AttenuationShapeExtents = FVector(300.0f, 0.0f, 0.0f);
        Attenuation.FalloffDistance = 4200.0f;
    }
    UE_LOG(
        LogEchoes,
        Display,
        TEXT("[ECHOES_AUDIO_READY] revision=presentation-audio-v1 cues=%d authored=%s command2D=true destruction3D=true commandCooldownMs=80 destructionCooldownMs=140 runtimeAuthority=presentation thirdPartySamples=false finalAudio=false commandMaxConcurrent=%d destructionMaxConcurrent=%d concurrencyPolicies=%s"),
        GetLoadedCueCount(),
        HasAllAuthoredCueAssets() && HasBoundedSpatialAttenuation() &&
                HasBoundedConcurrencyPolicies()
            ? TEXT("true") : TEXT("false"),
        GetCommandMaxConcurrentVoices(),
        GetDestructionMaxConcurrentVoices(),
        HasBoundedConcurrencyPolicies() ? TEXT("true") : TEXT("false"));
}

bool UEchoesPresentationAudioSubsystem::PlayCommandConfirmation()
{
    UWorld* World = GetWorld();
    const UEchoesGameUserSettings* Settings = UEchoesGameUserSettings::Get();
    USoundConcurrency* Concurrency = GetConcurrencyPolicy(
        EEchoesPresentationAudioCue::CommandConfirm);
    if (World == nullptr || Settings == nullptr || CommandConfirmSound == nullptr ||
        Concurrency == nullptr)
    {
        return false;
    }
    const float EffectsVolume = Settings->GetEffectsVolume();
    const double TimeSeconds = World->GetRealTimeSeconds();
    if (!ReserveCue(
            EEchoesPresentationAudioCue::CommandConfirm,
            TimeSeconds,
            EffectsVolume))
    {
        return false;
    }
    const float Volume = GetCueVolume(
        EEchoesPresentationAudioCue::CommandConfirm,
        EffectsVolume,
        Settings->IsReducedDynamicRangeEnabled());
    UGameplayStatics::PlaySound2D(
        World,
        CommandConfirmSound,
        Volume,
        1.0f,
        0.0f,
        Concurrency);
    ++SuccessfulCommandPlayCount;
    UE_LOG(
        LogEchoes,
        Verbose,
        TEXT("[ECHOES_AUDIO_EVENT] cue=command_confirm played=true spatial=false volume=%.2f authoritative=false"),
        Volume);
    return true;
}

bool UEchoesPresentationAudioSubsystem::PlayDestruction(
    echoes::sim::Faction Faction,
    const FVector& Location)
{
    EEchoesPresentationAudioCue Cue =
        EEchoesPresentationAudioCue::DestructionMeridian;
    switch (Faction)
    {
        case echoes::sim::Faction::MeridianCompact:
            break;
        case echoes::sim::Faction::KharuunAssemblies:
            Cue = EEchoesPresentationAudioCue::DestructionKharuun;
            break;
        case echoes::sim::Faction::HollowChoir:
            Cue = EEchoesPresentationAudioCue::DestructionChoir;
            break;
    }
    UWorld* World = GetWorld();
    const UEchoesGameUserSettings* Settings = UEchoesGameUserSettings::Get();
    USoundBase* Sound = GetCueAsset(Cue);
    USoundConcurrency* Concurrency = GetConcurrencyPolicy(Cue);
    if (World == nullptr || Settings == nullptr || Sound == nullptr ||
        Concurrency == nullptr)
    {
        return false;
    }
    const float EffectsVolume = Settings->GetEffectsVolume();
    const double TimeSeconds = World->GetRealTimeSeconds();
    if (!ReserveCue(Cue, TimeSeconds, EffectsVolume))
    {
        return false;
    }
    const float Volume = GetCueVolume(
        Cue,
        EffectsVolume,
        Settings->IsReducedDynamicRangeEnabled());
    UGameplayStatics::PlaySoundAtLocation(
        World,
        Sound,
        Location,
        FRotator::ZeroRotator,
        Volume,
        1.0f,
        0.0f,
        DestructionAttenuation,
        Concurrency);
    ++SuccessfulDestructionPlayCount;
    UE_LOG(
        LogEchoes,
        Verbose,
        TEXT("[ECHOES_AUDIO_EVENT] cue=%s played=true spatial=true volume=%.2f authoritative=false"),
        CueStableName(Cue),
        Volume);
    return true;
}

bool UEchoesPresentationAudioSubsystem::HasAllAuthoredCueAssets() const
{
    return CommandConfirmSound != nullptr &&
        MeridianDestructionSound != nullptr &&
        KharuunDestructionSound != nullptr &&
        ChoirDestructionSound != nullptr;
}

int32 UEchoesPresentationAudioSubsystem::GetLoadedCueCount() const
{
    return (CommandConfirmSound != nullptr ? 1 : 0) +
        (MeridianDestructionSound != nullptr ? 1 : 0) +
        (KharuunDestructionSound != nullptr ? 1 : 0) +
        (ChoirDestructionSound != nullptr ? 1 : 0);
}

bool UEchoesPresentationAudioSubsystem::HasBoundedSpatialAttenuation() const
{
    return DestructionAttenuation != nullptr &&
        DestructionAttenuation->Attenuation.bAttenuate &&
        DestructionAttenuation->Attenuation.bSpatialize &&
        DestructionAttenuation->Attenuation.AttenuationShape ==
            EAttenuationShape::Sphere &&
        DestructionAttenuation->Attenuation.FalloffDistance > 0.0f;
}

bool UEchoesPresentationAudioSubsystem::HasBoundedConcurrencyPolicies() const
{
    if (CommandConcurrency == nullptr || DestructionConcurrency == nullptr)
    {
        return false;
    }

    const FSoundConcurrencySettings& Command = CommandConcurrency->Concurrency;
    const FSoundConcurrencySettings& Destruction =
        DestructionConcurrency->Concurrency;
    return CommandConcurrency->GetOuter() == this &&
        DestructionConcurrency->GetOuter() == this &&
        CommandConcurrency->HasAnyFlags(RF_Transient) &&
        DestructionConcurrency->HasAnyFlags(RF_Transient) &&
        Command.MaxCount == GetCommandMaxConcurrentVoices() &&
        !Command.bLimitToOwner &&
        Command.ResolutionRule == EMaxConcurrentResolutionRule::PreventNew &&
        Command.RetriggerTime == 0.0f &&
        Destruction.MaxCount == GetDestructionMaxConcurrentVoices() &&
        !Destruction.bLimitToOwner &&
        Destruction.ResolutionRule ==
            EMaxConcurrentResolutionRule::StopFarthestThenOldest &&
        Destruction.RetriggerTime == 0.0f;
}

bool UEchoesPresentationAudioSubsystem::ReserveCue(
    EEchoesPresentationAudioCue Cue,
    double TimeSeconds,
    float EffectsVolume)
{
    if (!FMath::IsFinite(EffectsVolume) ||
        EffectsVolume < MinimumAudibleEffectsVolume ||
        !FMath::IsFinite(TimeSeconds))
    {
        return false;
    }
    double& LastSeconds =
        Cue == EEchoesPresentationAudioCue::CommandConfirm
            ? LastCommandSeconds
            : LastDestructionSeconds;
    const double Cooldown =
        Cue == EEchoesPresentationAudioCue::CommandConfirm
            ? GetCommandCooldownSeconds()
            : GetDestructionCooldownSeconds();
    if (TimeSeconds < LastSeconds || TimeSeconds - LastSeconds < Cooldown)
    {
        return false;
    }
    LastSeconds = TimeSeconds;
    return true;
}

float UEchoesPresentationAudioSubsystem::GetCueVolume(
    EEchoesPresentationAudioCue Cue,
    float EffectsVolume,
    bool bReducedDynamicRange) const
{
    const float SafeEffectsVolume = FMath::Clamp(EffectsVolume, 0.0f, 1.0f);
    if (Cue == EEchoesPresentationAudioCue::CommandConfirm)
    {
        return SafeEffectsVolume * (bReducedDynamicRange ? 0.68f : 0.56f);
    }
    return SafeEffectsVolume * (bReducedDynamicRange ? 0.74f : 0.96f);
}

USoundBase* UEchoesPresentationAudioSubsystem::GetCueAsset(
    EEchoesPresentationAudioCue Cue) const
{
    switch (Cue)
    {
        case EEchoesPresentationAudioCue::DestructionMeridian:
            return MeridianDestructionSound;
        case EEchoesPresentationAudioCue::DestructionKharuun:
            return KharuunDestructionSound;
        case EEchoesPresentationAudioCue::DestructionChoir:
            return ChoirDestructionSound;
        case EEchoesPresentationAudioCue::CommandConfirm:
        default:
            return CommandConfirmSound;
    }
}

USoundConcurrency* UEchoesPresentationAudioSubsystem::GetConcurrencyPolicy(
    EEchoesPresentationAudioCue Cue) const
{
    return Cue == EEchoesPresentationAudioCue::CommandConfirm
        ? CommandConcurrency.Get()
        : DestructionConcurrency.Get();
}

#if WITH_DEV_AUTOMATION_TESTS
bool UEchoesPresentationAudioSubsystem::ReserveCueForTest(
    EEchoesPresentationAudioCue Cue,
    double TimeSeconds,
    float EffectsVolume)
{
    return ReserveCue(Cue, TimeSeconds, EffectsVolume);
}

void UEchoesPresentationAudioSubsystem::ResetRateLimitsForTest()
{
    LastCommandSeconds = -1000.0;
    LastDestructionSeconds = -1000.0;
    SuccessfulCommandPlayCount = 0;
    SuccessfulDestructionPlayCount = 0;
}

float UEchoesPresentationAudioSubsystem::GetCueVolumeForTest(
    EEchoesPresentationAudioCue Cue,
    float EffectsVolume,
    bool bReducedDynamicRange) const
{
    return GetCueVolume(Cue, EffectsVolume, bReducedDynamicRange);
}

const USoundConcurrency*
UEchoesPresentationAudioSubsystem::GetConcurrencyPolicyForTest(
    EEchoesPresentationAudioCue Cue) const
{
    return GetConcurrencyPolicy(Cue);
}
#endif

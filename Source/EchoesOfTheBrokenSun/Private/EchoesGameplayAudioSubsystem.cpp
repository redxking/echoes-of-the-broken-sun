#include "EchoesGameplayAudioSubsystem.h"

#include "EchoesAudioMixSubsystem.h"
#include "EchoesGameUserSettings.h"
#include "EchoesOfTheBrokenSun.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundAttenuation.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundSubmix.h"

namespace
{
constexpr int32 GameplayEventCount =
    static_cast<int32>(EEchoesGameplayAudioEvent::Count);

/** Index-aligned with EEchoesGameplayAudioEvent. The coverage contract. */
constexpr const TCHAR* EventCuePaths[GameplayEventCount] = {
    TEXT("/Game/Audio/Generated/SFX_WeaponLight.SFX_WeaponLight"),
    TEXT("/Game/Audio/Generated/SFX_WeaponLine.SFX_WeaponLine"),
    TEXT("/Game/Audio/Generated/SFX_WeaponHeavy.SFX_WeaponHeavy"),
    TEXT("/Game/Audio/Generated/SFX_ImpactHit.SFX_ImpactHit"),
    TEXT("/Game/Audio/Generated/SFX_ImpactShielded.SFX_ImpactShielded"),
    TEXT("/Game/Audio/Generated/SFX_GatherMatter.SFX_GatherMatter"),
    TEXT("/Game/Audio/Generated/SFX_DeliverMatter.SFX_DeliverMatter"),
    TEXT("/Game/Audio/Generated/SFX_ConstructionStart.SFX_ConstructionStart"),
    TEXT(
        "/Game/Audio/Generated/SFX_ConstructionComplete.SFX_ConstructionComplete"),
    TEXT(
        "/Game/Audio/Generated/SFX_ProductionComplete.SFX_ProductionComplete"),
    TEXT("/Game/Audio/Generated/SFX_ResearchStart.SFX_ResearchStart"),
    TEXT(
        "/Game/Audio/Generated/SFX_ResearchInterrupted.SFX_ResearchInterrupted"),
    TEXT("/Game/Audio/Generated/SFX_WellClaim.SFX_WellClaim"),
    TEXT("/Game/Audio/Generated/SFX_WellHarvest.SFX_WellHarvest"),
    TEXT("/Game/Audio/Generated/SFX_WellPreserve.SFX_WellPreserve"),
    TEXT("/Game/Audio/Generated/SFX_WellReshape.SFX_WellReshape"),
    TEXT("/Game/Audio/Generated/SFX_ReshapeOpen.SFX_ReshapeOpen"),
    TEXT("/Game/Audio/Generated/SFX_ReshapeClose.SFX_ReshapeClose"),
};
}

void UEchoesGameplayAudioSubsystem::Initialize(
    FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    Collection.InitializeDependency(UEchoesAudioMixSubsystem::StaticClass());

    EventCues.SetNum(GameplayEventCount);
    for (int32 Index = 0; Index < GameplayEventCount; ++Index)
    {
        EventCues[Index] = LoadCue(EventCuePaths[Index]);
    }
    for (double& Last : LastEventSeconds)
    {
        Last = -1000.0;
    }

    // Battlefield events share one bounded linear falloff, mirroring the
    // accepted destruction-cue spatial policy.
    SpatialAttenuation = NewObject<USoundAttenuation>(this);
    if (SpatialAttenuation != nullptr)
    {
        FSoundAttenuationSettings& Attenuation =
            SpatialAttenuation->Attenuation;
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
        TEXT("[ECHOES_GAMEPLAY_AUDIO_READY] events=%d authored=%s routing=%s spatial=%s runtimeAuthority=presentation finalAudio=false"),
        GameplayEventCount,
        HasAllAuthoredCues() ? TEXT("true") : TEXT("false"),
        HasEffectsSubmixRouting() ? TEXT("true") : TEXT("false"),
        HasBoundedSpatialAttenuation() ? TEXT("true") : TEXT("false"));
}

void UEchoesGameplayAudioSubsystem::Deinitialize()
{
    // The cues are standalone assets that outlive this world; a stale
    // SoundSubmixObject would pin the per-world mix graph and trip the
    // editor's world-leak check.
    for (TObjectPtr<USoundBase>& Cue : EventCues)
    {
        if (Cue != nullptr)
        {
            Cue->SoundSubmixObject = nullptr;
        }
    }
    EventCues.Reset();
    Super::Deinitialize();
}

USoundBase* UEchoesGameplayAudioSubsystem::LoadCue(const TCHAR* Path)
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
            if (USoundSubmix* EffectsSubmix =
                    Mix->GetCategorySubmix(EEchoesAudioCategory::Effects))
            {
                Sound->SoundSubmixObject = EffectsSubmix;
            }
        }
    }
    return Sound;
}

EEchoesGameplayAudioEvent UEchoesGameplayAudioSubsystem::WeaponEventForType(
    echoes::sim::EntityType Type)
{
    switch (Type)
    {
        case echoes::sim::EntityType::Worker:
        case echoes::sim::EntityType::ScoutUnit:
            return EEchoesGameplayAudioEvent::WeaponFireLight;
        case echoes::sim::EntityType::HeavyUnit:
            return EEchoesGameplayAudioEvent::WeaponFireHeavy;
        case echoes::sim::EntityType::Soldier:
        default:
            return EEchoesGameplayAudioEvent::WeaponFireLine;
    }
}

EEchoesGameplayAudioEvent UEchoesGameplayAudioSubsystem::WellEventForChoice(
    echoes::sim::FutureWellChoice Choice)
{
    switch (Choice)
    {
        case echoes::sim::FutureWellChoice::Harvest:
            return EEchoesGameplayAudioEvent::WellHarvest;
        case echoes::sim::FutureWellChoice::Preserve:
            return EEchoesGameplayAudioEvent::WellPreserve;
        case echoes::sim::FutureWellChoice::Reshape:
        default:
            return EEchoesGameplayAudioEvent::WellReshape;
    }
}

float UEchoesGameplayAudioSubsystem::GetEventCooldownSeconds(
    EEchoesGameplayAudioEvent Event)
{
    switch (Event)
    {
        // Combat classes fire constantly under load; tight shared windows
        // keep them present without stacking a wall of voices.
        case EEchoesGameplayAudioEvent::WeaponFireLight:
        case EEchoesGameplayAudioEvent::WeaponFireLine:
        case EEchoesGameplayAudioEvent::WeaponFireHeavy:
            return 0.09f;
        case EEchoesGameplayAudioEvent::ImpactHit:
        case EEchoesGameplayAudioEvent::ImpactShielded:
            return 0.07f;
        case EEchoesGameplayAudioEvent::GatherMatter:
        case EEchoesGameplayAudioEvent::DeliverMatter:
            return 0.25f;
        // Singular events read as punctuation; a longer window suffices.
        default:
            return 0.4f;
    }
}

bool UEchoesGameplayAudioSubsystem::AreEffectsAudible() const
{
    const UEchoesGameUserSettings* Settings = UEchoesGameUserSettings::Get();
    return Settings != nullptr && Settings->GetEffectsVolume() > 0.0f &&
        Settings->GetMasterVolume() > 0.0f;
}

bool UEchoesGameplayAudioSubsystem::ReserveEvent(
    EEchoesGameplayAudioEvent Event,
    double Seconds)
{
    if (!FMath::IsFinite(Seconds) || !AreEffectsAudible())
    {
        return false;
    }
    const int32 Index = static_cast<int32>(Event);
    if (Index < 0 || Index >= GameplayEventCount)
    {
        return false;
    }
    double& Last = LastEventSeconds[Index];
    if (Seconds < Last ||
        Seconds - Last < GetEventCooldownSeconds(Event))
    {
        return false;
    }
    Last = Seconds;
    return true;
}

USoundBase* UEchoesGameplayAudioSubsystem::ResolveEventCue(
    EEchoesGameplayAudioEvent Event) const
{
    const int32 Index = static_cast<int32>(Event);
    return EventCues.IsValidIndex(Index) ? EventCues[Index].Get() : nullptr;
}

bool UEchoesGameplayAudioSubsystem::PlayEvent(
    EEchoesGameplayAudioEvent Event,
    const FVector& WorldLocation)
{
    UWorld* World = GetWorld();
    USoundBase* Sound = ResolveEventCue(Event);
    if (World == nullptr || Sound == nullptr ||
        !ReserveEvent(Event, World->GetRealTimeSeconds()))
    {
        return false;
    }
    UGameplayStatics::PlaySoundAtLocation(
        World,
        Sound,
        WorldLocation,
        FRotator::ZeroRotator,
        1.0f,
        1.0f,
        0.0f,
        SpatialAttenuation);
    ++PlayCount;
    return true;
}

bool UEchoesGameplayAudioSubsystem::PlayEvent2D(
    EEchoesGameplayAudioEvent Event)
{
    UWorld* World = GetWorld();
    USoundBase* Sound = ResolveEventCue(Event);
    if (World == nullptr || Sound == nullptr ||
        !ReserveEvent(Event, World->GetRealTimeSeconds()))
    {
        return false;
    }
    UGameplayStatics::PlaySound2D(World, Sound);
    ++PlayCount;
    return true;
}

bool UEchoesGameplayAudioSubsystem::HasAllAuthoredCues() const
{
    return GetLoadedCueCount() == GameplayEventCount;
}

int32 UEchoesGameplayAudioSubsystem::GetLoadedCueCount() const
{
    int32 Count = 0;
    for (const TObjectPtr<USoundBase>& Cue : EventCues)
    {
        if (Cue != nullptr)
        {
            ++Count;
        }
    }
    return Count;
}

bool UEchoesGameplayAudioSubsystem::HasEffectsSubmixRouting() const
{
    UWorld* World = GetWorld();
    UEchoesAudioMixSubsystem* Mix =
        World != nullptr ? World->GetSubsystem<UEchoesAudioMixSubsystem>()
                         : nullptr;
    USoundSubmix* EffectsSubmix =
        Mix != nullptr ? Mix->GetCategorySubmix(EEchoesAudioCategory::Effects)
                       : nullptr;
    if (EffectsSubmix == nullptr || EventCues.IsEmpty())
    {
        return false;
    }
    for (const TObjectPtr<USoundBase>& Cue : EventCues)
    {
        if (Cue == nullptr || Cue->SoundSubmixObject != EffectsSubmix)
        {
            return false;
        }
    }
    return true;
}

bool UEchoesGameplayAudioSubsystem::HasBoundedSpatialAttenuation() const
{
    return SpatialAttenuation != nullptr &&
        SpatialAttenuation->Attenuation.bAttenuate &&
        SpatialAttenuation->Attenuation.bSpatialize &&
        SpatialAttenuation->Attenuation.AttenuationShape ==
            EAttenuationShape::Sphere &&
        SpatialAttenuation->Attenuation.FalloffDistance > 0.0f;
}

#if WITH_DEV_AUTOMATION_TESTS
bool UEchoesGameplayAudioSubsystem::ReserveEventForTest(
    EEchoesGameplayAudioEvent Event,
    double Seconds)
{
    return ReserveEvent(Event, Seconds);
}

void UEchoesGameplayAudioSubsystem::ResetRateLimitsForTest()
{
    for (double& Last : LastEventSeconds)
    {
        Last = -1000.0;
    }
    PlayCount = 0;
}
#endif

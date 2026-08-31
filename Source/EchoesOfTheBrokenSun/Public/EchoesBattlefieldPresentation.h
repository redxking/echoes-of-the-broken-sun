#pragma once

#include "CoreMinimal.h"
#include "EchoesSkirmishSetup.h"

/** Stable runtime tags consumed by code-only and authored map presentation. */
namespace EchoesBattlefieldPresentation
{
inline const FName& RootTag()
{
    static const FName Tag(TEXT("EchoesBattlefieldPresentation"));
    return Tag;
}

inline const FName& SharedTag()
{
    static const FName Tag(TEXT("EchoesBattlefieldShared"));
    return Tag;
}

inline const FName& FloorTag()
{
    static const FName Tag(TEXT("EchoesBattlefieldFloor"));
    return Tag;
}

inline const FName& SunTag()
{
    static const FName Tag(TEXT("EchoesBattlefieldSun"));
    return Tag;
}

inline const FName& SkyTag()
{
    static const FName Tag(TEXT("EchoesBattlefieldSky"));
    return Tag;
}

inline const FName& WeatherTag()
{
    static const FName Tag(TEXT("EchoesBattlefieldWeather"));
    return Tag;
}

inline const FName& FutureWellLandmarkTag()
{
    static const FName Tag(TEXT("EchoesMapFutureWellLandmark"));
    return Tag;
}

inline const FName& GateLandmarkTag()
{
    static const FName Tag(TEXT("EchoesMapGateLandmark"));
    return Tag;
}

inline const FName& GlassScarTag()
{
    static const FName Tag(TEXT("EchoesMapPresentationGlassScar"));
    return Tag;
}

inline const FName& CrownfallBasinTag()
{
    static const FName Tag(TEXT("EchoesMapPresentationCrownfallBasin"));
    return Tag;
}

inline const FName& SorynConfluenceTag()
{
    static const FName Tag(TEXT("EchoesMapPresentationSorynConfluence"));
    return Tag;
}

inline const FName& LegacyGlassScarTag()
{
    static const FName Tag(TEXT("EchoesGlassScarComposition"));
    return Tag;
}

inline const FName& TagForPreset(EEchoesSkirmishMapPreset Preset)
{
    switch (Preset)
    {
        case EEchoesSkirmishMapPreset::GlassScar:
            return GlassScarTag();
        case EEchoesSkirmishMapPreset::CrownfallBasin:
            return CrownfallBasinTag();
        case EEchoesSkirmishMapPreset::SorynConfluence:
            return SorynConfluenceTag();
    }
    return GlassScarTag();
}

inline void RegisterSharedActorTags(
    TArray<FName>& Tags,
    const FName& RoleTag)
{
    Tags.AddUnique(RootTag());
    Tags.AddUnique(SharedTag());
    Tags.AddUnique(RoleTag);
}

inline void RegisterPresetActorTags(
    TArray<FName>& Tags,
    EEchoesSkirmishMapPreset Preset)
{
    Tags.AddUnique(RootTag());
    Tags.AddUnique(TagForPreset(Preset));
}

inline bool HasPresetScope(const TArray<FName>& Tags)
{
    return Tags.Contains(GlassScarTag()) ||
           Tags.Contains(CrownfallBasinTag()) ||
           Tags.Contains(SorynConfluenceTag()) ||
           Tags.Contains(LegacyGlassScarTag());
}

inline bool IsRegistered(const TArray<FName>& Tags)
{
    return Tags.Contains(RootTag()) || Tags.Contains(LegacyGlassScarTag());
}

inline bool IsShared(const TArray<FName>& Tags)
{
    return Tags.Contains(RootTag()) && Tags.Contains(SharedTag()) &&
           !HasPresetScope(Tags);
}

inline bool ShouldShow(
    const TArray<FName>& Tags,
    EEchoesSkirmishMapPreset Preset)
{
    if (IsShared(Tags))
    {
        return true;
    }
    return Tags.Contains(TagForPreset(Preset)) ||
           (Preset == EEchoesSkirmishMapPreset::GlassScar &&
            Tags.Contains(LegacyGlassScarTag()));
}

inline const TCHAR* StableName(EEchoesSkirmishMapPreset Preset)
{
    switch (Preset)
    {
        case EEchoesSkirmishMapPreset::GlassScar:
            return TEXT("GLASS_SCAR");
        case EEchoesSkirmishMapPreset::CrownfallBasin:
            return TEXT("CROWNFALL_BASIN");
        case EEchoesSkirmishMapPreset::SorynConfluence:
            return TEXT("SORYN_CONFLUENCE");
    }
    return TEXT("GLASS_SCAR");
}
}

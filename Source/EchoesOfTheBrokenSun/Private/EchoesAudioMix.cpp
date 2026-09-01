#include "EchoesAudioMix.h"

namespace EchoesAudioMix
{
const TCHAR* CategoryStableName(EEchoesAudioCategory Category)
{
    switch (Category)
    {
        case EEchoesAudioCategory::Music:
            return TEXT("music");
        case EEchoesAudioCategory::Dialogue:
            return TEXT("dialogue");
        case EEchoesAudioCategory::Interface:
            return TEXT("interface");
        case EEchoesAudioCategory::Ambience:
            return TEXT("ambience");
        case EEchoesAudioCategory::Effects:
        default:
            return TEXT("effects");
    }
}

int32 CategoryIndex(EEchoesAudioCategory Category)
{
    return static_cast<int32>(Category);
}

float CategoryVolume(
    const FEchoesAudioMixVolumes& Volumes,
    EEchoesAudioCategory Category)
{
    float Raw = 0.0f;
    switch (Category)
    {
        case EEchoesAudioCategory::Music:
            Raw = Volumes.Music;
            break;
        case EEchoesAudioCategory::Dialogue:
            Raw = Volumes.Dialogue;
            break;
        case EEchoesAudioCategory::Interface:
            Raw = Volumes.Interface;
            break;
        case EEchoesAudioCategory::Ambience:
            Raw = Volumes.Ambience;
            break;
        case EEchoesAudioCategory::Effects:
        default:
            Raw = Volumes.Effects;
            break;
    }
    if (!FMath::IsFinite(Raw))
    {
        return 0.0f;
    }
    return FMath::Clamp(Raw, 0.0f, 1.0f);
}

float ResolveCategoryGain(
    const FEchoesAudioMixVolumes& Volumes,
    EEchoesAudioCategory Category,
    bool bReducedDynamicRange)
{
    const float Position = CategoryVolume(Volumes, Category);
    const float Master = FMath::IsFinite(Volumes.Master)
        ? FMath::Clamp(Volumes.Master, 0.0f, 1.0f)
        : 0.0f;

    // A muted category stays muted under every other setting, including
    // reduced dynamic range. Compression narrows a range; it never reopens a
    // control the player closed.
    if (Position <= 0.0f || Master <= 0.0f)
    {
        return 0.0f;
    }

    float Gain = Position;
    if (bReducedDynamicRange)
    {
        Gain = ReducedRangeReferenceGain +
            (Position - ReducedRangeReferenceGain) * ReducedRangeRetainedSpread;
        Gain = FMath::Clamp(Gain, 0.0f, 1.0f);
    }
    return FMath::Clamp(Gain * Master, 0.0f, 1.0f);
}
}

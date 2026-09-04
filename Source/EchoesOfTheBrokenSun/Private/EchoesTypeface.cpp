#include "EchoesTypeface.h"

#include "EchoesOfTheBrokenSun.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "Fonts/CompositeFont.h"
#include "Misc/Paths.h"
#include "UObject/StrongObjectPtr.h"

namespace
{
struct FEchoesTypefaceSlot
{
    const TCHAR* Label;
    const TCHAR* RelativeFile;
    int32 LegacySize;
    TStrongObjectPtr<UFont> Font;
    bool bVendored = false;
    bool bResolved = false;
};

FEchoesTypefaceSlot GChrome{
    TEXT("SpaceGrotesk"), TEXT("UI/Fonts/SpaceGrotesk/SpaceGrotesk[wght].ttf"), 10};
FEchoesTypefaceSlot GChromeLarge{
    TEXT("SpaceGrotesk"), TEXT("UI/Fonts/SpaceGrotesk/SpaceGrotesk[wght].ttf"), 14};
FEchoesTypefaceSlot GReadout{
    TEXT("IBMPlexMono"), TEXT("UI/Fonts/IBMPlexMono/IBMPlexMono-Medium.ttf"), 10};

UFont* EngineFallback(int32 LegacySize)
{
    if (GEngine == nullptr)
    {
        return nullptr;
    }
    return LegacySize >= 14 ? GEngine->GetMediumFont() : GEngine->GetSmallFont();
}

UFont* Resolve(FEchoesTypefaceSlot& Slot)
{
    if (Slot.bResolved)
    {
        return Slot.Font.IsValid() ? Slot.Font.Get() : EngineFallback(Slot.LegacySize);
    }
    Slot.bResolved = true;
    const FString Path = FPaths::ConvertRelativePathToFull(
        FPaths::Combine(FPaths::ProjectContentDir(), Slot.RelativeFile));
    if (!FPaths::FileExists(Path))
    {
        UE_LOG(
            LogEchoes,
            Warning,
            TEXT("[ECHOES_TYPEFACE_FALLBACK] face=%s size=%d file=%s reason=missing fallback=engine"),
            Slot.Label,
            Slot.LegacySize,
            *Path);
        return EngineFallback(Slot.LegacySize);
    }
    UFont* Font = NewObject<UFont>(GetTransientPackage(), NAME_None, RF_Transient);
    if (Font == nullptr)
    {
        return EngineFallback(Slot.LegacySize);
    }
    Font->FontCacheType = EFontCacheType::Runtime;
    Font->LegacyFontSize = Slot.LegacySize;
    Font->LegacyFontName = FName(Slot.Label);
    Font->CompositeFont.DefaultTypeface.AppendFont(
        TEXT("Regular"), Path, EFontHinting::Default, EFontLoadingPolicy::LazyLoad);
    Slot.Font.Reset(Font);
    Slot.bVendored = true;
    UE_LOG(
        LogEchoes,
        Display,
        TEXT("[ECHOES_TYPEFACE_READY] face=%s size=%d licence=SIL-OFL-1.1 file=%s vendored=true"),
        Slot.Label,
        Slot.LegacySize,
        *Path);
    return Font;
}
}  // namespace

namespace EchoesTypeface
{
UFont* Chrome()
{
    return Resolve(GChrome);
}

UFont* ChromeLarge()
{
    return Resolve(GChromeLarge);
}

UFont* Readout()
{
    return Resolve(GReadout);
}

bool IsVendoredTypefaceActive()
{
    return Resolve(GChrome) != nullptr && GChrome.bVendored &&
           Resolve(GChromeLarge) != nullptr && GChromeLarge.bVendored &&
           Resolve(GReadout) != nullptr && GReadout.bVendored;
}
}  // namespace EchoesTypeface

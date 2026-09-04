#pragma once

#include "CoreMinimal.h"

class UFont;

/**
 * Project typefaces (directive open decision 2, resolved): Space Grotesk for
 * interface chrome and IBM Plex Mono for tactical readouts, both SIL OFL 1.1,
 * vendored under Content/UI/Fonts with their licence files beside them.
 *
 * The fonts are built once as runtime-cached UFont objects straight from the
 * vendored TTF files, so no editor-authored Font asset is needed and the
 * files ship as staged content. If a file is missing the engine font is
 * returned instead and the fallback is logged, so text never disappears.
 * Presentation only: nothing here touches simulation state.
 */
namespace EchoesTypeface
{
/** Interface chrome at the small HUD size (replaces GEngine->GetSmallFont()). */
ECHOESOFTHEBROKENSUN_API UFont* Chrome();
/** Interface chrome at the medium size (replaces GEngine->GetMediumFont()). */
ECHOESOFTHEBROKENSUN_API UFont* ChromeLarge();
/** Monospaced readouts: resource ledger, ticks, coordinates. */
ECHOESOFTHEBROKENSUN_API UFont* Readout();
/** True when every face resolved to a vendored file rather than a fallback. */
ECHOESOFTHEBROKENSUN_API bool IsVendoredTypefaceActive();
}  // namespace EchoesTypeface

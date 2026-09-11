// Author: Angelis Pseftis
#pragma once
#include "CoreMinimal.h"
#include "InputCoreTypes.h"
#include "EchoesCommandDeckModel.h"

/** Resolves active bindings at presentation time; never mutates input settings. */
struct ECHOESOFTHEBROKENSUN_API FEchoesInputPrompt final
{
    static FText Chord(const FKey& Key, bool bShift, bool bCtrl, bool bAlt, bool bCmd);
    static FText Action(FName Name);
    static FText Axis(FName Name);
    static FText Command(EEchoesCommandDeckAction Action);
    /**
     * Chord with the key's short name (";" rather than "Semicolon") for the
     * corner of a command tile, where the long name of a punctuation key ran
     * into the neighbouring tile. Modifier prefixes are unchanged.
     */
    static FText Glyph(const FKey& Key, bool bShift, bool bCtrl, bool bAlt, bool bCmd);
    static FText CommandGlyph(EEchoesCommandDeckAction Action);
};

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
};

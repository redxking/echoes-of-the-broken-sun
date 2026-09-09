// Copyright Echoes of the Broken Sun. Author: Angelis Pseftis
#pragma once

#include "CoreMinimal.h"
#include "EchoesBuildPlacementPreview.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

/** Opt-in diagnostics only. No serialized state and no gameplay decisions. */
struct FEchoesPlacementDiagnostics final
{
    static bool Enabled()
    {
        static const bool bEnabled = FParse::Param(FCommandLine::Get(), TEXT("EchoesPlacementTrace"));
        return bEnabled;
    }

    static FString Preview(const echoes::sim::PlayerView& View,
        uint32 Worker, echoes::sim::EntityType Type, echoes::sim::Vec2 Position,
        const FVector2D& Pointer, const FVector2D& Viewport, const FVector& World,
        const FEchoesBuildPlacementEvaluation& Evaluation, bool bHasGround)
    {
        return FString::Printf(TEXT("tick=%llu worker=%u type=%u pointer=(%.3f,%.3f) viewport=(%.0f,%.0f) world=(%.3f,%.3f,%.3f) raw=(%d,%d) ground=%d validity=%u connects=%d node=%u radius_raw=%d matter=%d dawn=%d logistics=%d/%d"),
            static_cast<unsigned long long>(View.CurrentTick()), Worker, static_cast<uint32>(Type),
            Pointer.X, Pointer.Y, Viewport.X, Viewport.Y, World.X, World.Y, World.Z,
            Position.x.Raw(), Position.y.Raw(), bHasGround, static_cast<uint32>(Evaluation.Validity),
            Evaluation.bWillConnect, Evaluation.ConnectionNodeId, Evaluation.ConnectionRadiusRaw,
            View.Player().resources.material, View.Player().resources.dawnshards,
            View.PopulationUsed(), View.PopulationCapacity());
    }
};

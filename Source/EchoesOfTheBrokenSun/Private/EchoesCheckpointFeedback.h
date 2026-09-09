#pragma once

#include "EchoesSimulationSubsystem.h"

namespace EchoesCheckpointFeedback
{
// Keep transaction diagnostics in the retained status/log. The player needs
// the actual lifecycle result, not storage codes, simulation ticks or paths.
inline FText Display(const FEchoesCheckpointSaveStatus& Status)
{
    switch (Status.State)
    {
    case EEchoesCheckpointSaveState::Pending:
        return NSLOCTEXT("EchoesCheckpoint", "Saving", "Saving checkpoint…");
    case EEchoesCheckpointSaveState::Succeeded:
        return Status.bAutosave
            ? NSLOCTEXT("EchoesCheckpoint", "Autosaved", "Progress saved automatically.")
            : NSLOCTEXT("EchoesCheckpoint", "Saved", "Checkpoint saved.");
    case EEchoesCheckpointSaveState::Failed:
        return NSLOCTEXT("EchoesCheckpoint", "Failed", "Could not save the checkpoint. Please try again.");
    default:
        return FText::GetEmpty();
    }
}

inline FText Restored()
{
    return NSLOCTEXT("EchoesCheckpoint", "Restored", "Checkpoint restored. Ready for your command.");
}
}

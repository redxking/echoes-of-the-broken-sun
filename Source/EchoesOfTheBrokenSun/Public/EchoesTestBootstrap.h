#pragma once

#include "CoreMinimal.h"

/**
 * Validates the process-level persistence boundary used by unattended
 * regression runs. This is deliberately a launch check, not a replacement
 * for the campaign store's transactional validation.
 */
namespace EchoesTestBootstrap
{
    /**
     * Returns false when an automation launch lacks a valid sandbox manifest
     * below the fixed project automation root or legacy platform temp root.
     * Normal player launches return true unchanged.
     */
    ECHOESOFTHEBROKENSUN_API bool ValidateBeforeGameInstance(
        FString& OutFailure);

    /** True only after this module accepted the dedicated test launch routes. */
    ECHOESOFTHEBROKENSUN_API bool IsDedicatedTestSandboxValidated();

    /** Returns the canonical suite save root accepted during bootstrap. */
    ECHOESOFTHEBROKENSUN_API bool GetValidatedSuiteSaveDirectory(
        FString& OutSaveDirectory);
}

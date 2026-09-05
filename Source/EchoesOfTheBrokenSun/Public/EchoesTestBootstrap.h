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
     * Returns false when an automation launch lacks a valid, temporary
     * sandbox manifest. Normal player launches return true unchanged.
     */
    ECHOESOFTHEBROKENSUN_API bool ValidateBeforeGameInstance(
        FString& OutFailure);

    /** True only after this module accepted the dedicated test launch routes. */
    ECHOESOFTHEBROKENSUN_API bool IsDedicatedTestSandboxValidated();
}

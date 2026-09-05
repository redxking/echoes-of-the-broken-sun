#include "EchoesOfTheBrokenSun.h"
#include "EchoesTestBootstrap.h"
#include "Modules/ModuleManager.h"

DEFINE_LOG_CATEGORY(LogEchoes);

class FEchoesGameModule final : public FDefaultGameModuleImpl
{
public:
    virtual void StartupModule() override
    {
        FString Failure;
        if (!EchoesTestBootstrap::ValidateBeforeGameInstance(Failure))
        {
            UE_LOG(LogEchoes, Fatal, TEXT("%s Test launch aborted before GameInstance creation."), *Failure);
        }
        FDefaultGameModuleImpl::StartupModule();
    }
};

IMPLEMENT_PRIMARY_GAME_MODULE(
    FEchoesGameModule,
    EchoesOfTheBrokenSun,
    "EchoesOfTheBrokenSun");

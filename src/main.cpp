#include "PCH.h"

#include "Logger.h"
#include "Plugin.h"

SKSEPluginLoad(const SKSE::LoadInterface* a_skse)
{
    SKSE::Init(a_skse);

    if (!DialogueHUDFade::Logger::Initialize()) {
        return false;
    }

    return DialogueHUDFade::Plugin::Initialize(a_skse);
}

#pragma once

#include "Settings.h"

namespace DialogueHUDFade::Plugin
{
    constexpr std::string_view NAME = "DialogueHUDFade";
    constexpr std::string_view VERSION = "1.0.0";

    bool Initialize(const SKSE::LoadInterface* a_skse);
    void OnSKSEMessage(SKSE::MessagingInterface::Message* a_message);

    void OnDialogueOpen();
    void OnDialogueClose();
    void OnMenuReset();
    void OnFrame(float a_deltaSeconds);

    [[nodiscard]] const Settings& GetSettings();
}

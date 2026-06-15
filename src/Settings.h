#pragma once

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace DialogueHUDFade
{
    enum class Easing
    {
        kSmoothstep,
        kEaseOutCubic,
        kLinear
    };

    enum class TargetMode
    {
        kFullHUD
    };

    struct Settings
    {
        bool enabled{ true };
        bool fadeOnDialogue{ true };
        bool restoreOnDialogueEnd{ true };
        bool debugLogging{ false };

        float fadeOutSeconds{ 0.20F };
        float fadeInSeconds{ 0.20F };
        float hiddenAlpha{ 0.0F };
        float visibleAlpha{ 100.0F };
        Easing easing{ Easing::kSmoothstep };

        bool respectExternalHudChanges{ true };
        bool restoreOnlyIfChangedByPlugin{ true };
        bool retryIfHUDMenuMissing{ true };
        bool disableInMenus{ true };

        TargetMode targetMode{ TargetMode::kFullHUD };
        std::vector<std::string> fullHudPaths{ "_root.HUDMovieBaseInstance", "_root" };
    };

    struct SettingsLoadResult
    {
        Settings settings{};
        bool loadedFromFile{ false };
        std::vector<std::string> warnings{};
    };

    [[nodiscard]] SettingsLoadResult LoadSettings(const std::filesystem::path& a_path);
    [[nodiscard]] SettingsLoadResult ParseSettings(std::string_view a_iniText);

    [[nodiscard]] const char* ToString(Easing a_easing) noexcept;
    [[nodiscard]] Easing EasingFromString(std::string_view a_value, std::vector<std::string>* a_warnings = nullptr);
}

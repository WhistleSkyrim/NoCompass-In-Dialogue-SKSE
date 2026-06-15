#pragma once

#include "HudFader.h"
#include "Settings.h"

namespace DialogueHUDFade
{
    class ScaleformHudTarget final : public IHudTarget
    {
    public:
        explicit ScaleformHudTarget(const Settings& a_settings);

        [[nodiscard]] HudState Read() override;
        bool ApplyAlpha(float a_alpha) override;

    private:
        struct TargetPath
        {
            std::string objectPath;
            std::string alphaPath;
            std::string visiblePath;
            bool loggedMissing{ false };
        };

        [[nodiscard]] RE::GPtr<RE::GFxMovieView> GetMovie() const;
        [[nodiscard]] bool IsPathAvailable(RE::GFxMovieView& a_movie, TargetPath& a_path);
        void LogSelectedPath(const TargetPath& a_path);

        std::vector<TargetPath> paths_;
        bool debugLogging_{ false };
        std::string selectedAlphaPath_;
    };
}

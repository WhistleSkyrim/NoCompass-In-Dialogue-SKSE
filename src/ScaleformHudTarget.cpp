#include "PCH.h"

#include "ScaleformHudTarget.h"

namespace DialogueHUDFade
{
    namespace
    {
        [[nodiscard]] std::string BuildPropertyPath(const std::string& a_objectPath, std::string_view a_property)
        {
            auto path = a_objectPath;
            if (!path.empty() && path.back() != '.') {
                path.push_back('.');
            }
            path.append(a_property);
            return path;
        }
    }

    ScaleformHudTarget::ScaleformHudTarget(const Settings& a_settings) :
        debugLogging_(a_settings.debugLogging)
    {
        paths_.reserve(a_settings.fullHudPaths.size());
        for (const auto& path : a_settings.fullHudPaths) {
            if (path.empty()) {
                continue;
            }

            paths_.push_back(TargetPath{
                .objectPath = path,
                .alphaPath = BuildPropertyPath(path, "_alpha"),
                .visiblePath = BuildPropertyPath(path, "_visible"),
            });
        }

        if (paths_.empty()) {
            paths_.push_back(TargetPath{
                .objectPath = "_root",
                .alphaPath = "_root._alpha",
                .visiblePath = "_root._visible",
            });
        }
    }

    HudState ScaleformHudTarget::Read()
    {
        HudState result{};
        auto movie = GetMovie();
        if (!movie) {
            return result;
        }

        for (auto& path : paths_) {
            if (!IsPathAvailable(*movie, path)) {
                continue;
            }

            result.available = true;

            RE::GFxValue alpha;
            if (movie->GetVariable(std::addressof(alpha), path.alphaPath.c_str()) && alpha.IsNumber()) {
                result.alpha = ClampAlpha(static_cast<float>(alpha.GetNumber()));
                LogSelectedPath(path);
            }

            RE::GFxValue visible;
            if (movie->GetVariable(std::addressof(visible), path.visiblePath.c_str()) && visible.IsBool()) {
                result.visible = visible.GetBool();
            }

            if (result.alpha || result.visible) {
                return result;
            }
        }

        return result;
    }

    bool ScaleformHudTarget::ApplyAlpha(float a_alpha)
    {
        auto movie = GetMovie();
        if (!movie) {
            return false;
        }

        const RE::GFxValue alpha(ClampAlpha(a_alpha));

        // Treat fullHudPaths as an ordered fallback list and only drive the first
        // path that accepts the write. Writing to every available path (e.g. both
        // _root.HUDMovieBaseInstance and its _root parent) would dim the HUD twice
        // because Scaleform alpha multiplies down the display tree. This also keeps
        // ApplyAlpha consistent with Read, which uses the first matching path.
        for (auto& path : paths_) {
            if (!IsPathAvailable(*movie, path)) {
                continue;
            }

            if (movie->SetVariable(path.alphaPath.c_str(), alpha)) {
                LogSelectedPath(path);
                return true;
            }
        }

        return false;
    }

    RE::GPtr<RE::GFxMovieView> ScaleformHudTarget::GetMovie() const
    {
        const auto ui = RE::UI::GetSingleton();
        if (!ui) {
            return nullptr;
        }

        auto movie = ui->GetMovieView(RE::HUDMenu::MENU_NAME);
        if (!movie) {
            movie = ui->GetMovieView("HUD Menu");
        }

        return movie;
    }

    bool ScaleformHudTarget::IsPathAvailable(RE::GFxMovieView& a_movie, TargetPath& a_path)
    {
        RE::GFxValue object;
        if (a_movie.GetVariable(std::addressof(object), a_path.objectPath.c_str()) && (object.IsDisplayObject() || object.IsObject())) {
            return true;
        }

        if (a_movie.IsAvailable(a_path.objectPath.c_str())) {
            return true;
        }

        if (debugLogging_ && !a_path.loggedMissing) {
            spdlog::debug("HUD Scaleform path missing: {}", a_path.objectPath);
            a_path.loggedMissing = true;
        }

        return false;
    }

    void ScaleformHudTarget::LogSelectedPath(const TargetPath& a_path)
    {
        if (!debugLogging_ || selectedAlphaPath_ == a_path.alphaPath) {
            return;
        }

        selectedAlphaPath_ = a_path.alphaPath;
        spdlog::debug("Using HUD alpha path: {}", selectedAlphaPath_);
    }
}

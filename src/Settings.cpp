#include "Settings.h"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <cmath>
#include <fstream>
#include <memory>
#include <sstream>
#include <unordered_map>
#include <utility>

namespace DialogueHUDFade
{
    namespace
    {
        using SectionMap = std::unordered_map<std::string, std::unordered_map<std::string, std::string>>;

        [[nodiscard]] std::string Trim(std::string_view a_value)
        {
            auto begin = a_value.begin();
            auto end = a_value.end();

            while (begin != end && std::isspace(static_cast<unsigned char>(*begin)) != 0) {
                ++begin;
            }

            while (begin != end && std::isspace(static_cast<unsigned char>(*(end - 1))) != 0) {
                --end;
            }

            return { begin, end };
        }

        [[nodiscard]] std::size_t FindCommentStart(std::string_view a_line) noexcept
        {
            bool inSingleQuote = false;
            bool inDoubleQuote = false;

            for (std::size_t i = 0; i < a_line.size(); ++i) {
                const auto ch = a_line[i];
                if (ch == '\'' && !inDoubleQuote) {
                    inSingleQuote = !inSingleQuote;
                    continue;
                }
                if (ch == '"' && !inSingleQuote) {
                    inDoubleQuote = !inDoubleQuote;
                    continue;
                }

                if (inSingleQuote || inDoubleQuote) {
                    continue;
                }

                if (ch == ';') {
                    return i;
                }

                if (ch == '#') {
                    const auto startsComment = i == 0 || std::isspace(static_cast<unsigned char>(a_line[i - 1])) != 0;
                    if (startsComment) {
                        return i;
                    }
                }
            }

            return std::string_view::npos;
        }

        [[nodiscard]] std::string Unquote(std::string_view a_value)
        {
            const auto trimmed = Trim(a_value);
            if (trimmed.size() < 2) {
                return trimmed;
            }

            const auto first = trimmed.front();
            const auto last = trimmed.back();
            if ((first == '"' && last == '"') || (first == '\'' && last == '\'')) {
                return Trim(std::string_view(trimmed).substr(1, trimmed.size() - 2));
            }

            return trimmed;
        }

        [[nodiscard]] std::string Lower(std::string_view a_value)
        {
            std::string out;
            out.reserve(a_value.size());
            for (const char ch : a_value) {
                out.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(ch))));
            }
            return out;
        }

        [[nodiscard]] bool ParseBool(std::string_view a_value, bool a_default, std::vector<std::string>& a_warnings, std::string_view a_key)
        {
            const auto lower = Lower(Unquote(a_value));
            if (lower == "true" || lower == "1" || lower == "yes" || lower == "on") {
                return true;
            }
            if (lower == "false" || lower == "0" || lower == "no" || lower == "off") {
                return false;
            }

            a_warnings.emplace_back(std::string(a_key) + " had an invalid boolean value; default used");
            return a_default;
        }

        [[nodiscard]] float ParseFloat(std::string_view a_value, float a_default, float a_min, float a_max, std::vector<std::string>& a_warnings, std::string_view a_key)
        {
            const auto trimmed = Unquote(a_value);
            float parsed = a_default;
            const auto* first = trimmed.data();
            const auto* last = trimmed.data() + trimmed.size();
            const auto [ptr, ec] = std::from_chars(first, last, parsed);
            if (ec != std::errc{} || ptr != last || !std::isfinite(parsed)) {
                a_warnings.emplace_back(std::string(a_key) + " had an invalid numeric value; default used");
                return a_default;
            }

            const auto clamped = std::clamp(parsed, a_min, a_max);
            if (clamped != parsed) {
                a_warnings.emplace_back(std::string(a_key) + " was outside the supported range and was clamped");
            }
            return clamped;
        }

        [[nodiscard]] std::vector<std::string> SplitCsv(std::string_view a_value)
        {
            std::vector<std::string> out;
            std::string current;
            auto flushToken = [&out](std::string& a_token) {
                auto trimmed = Unquote(a_token);
                if (!trimmed.empty()) {
                    out.emplace_back(std::move(trimmed));
                }
                a_token.clear();
            };

            bool inSingleQuote = false;
            bool inDoubleQuote = false;
            for (const char ch : a_value) {
                if (ch == '\'' && !inDoubleQuote) {
                    inSingleQuote = !inSingleQuote;
                    current.push_back(ch);
                    continue;
                }
                if (ch == '"' && !inSingleQuote) {
                    inDoubleQuote = !inDoubleQuote;
                    current.push_back(ch);
                    continue;
                }
                if (ch == ',' && !inSingleQuote && !inDoubleQuote) {
                    flushToken(current);
                    continue;
                }

                current.push_back(ch);
            }

            flushToken(current);
            return out;
        }

        [[nodiscard]] SectionMap ParseIniMap(std::string_view a_iniText)
        {
            SectionMap map;
            std::string section = "general";
            std::istringstream stream{ std::string(a_iniText) };
            std::string line;

            while (std::getline(stream, line)) {
                if (!line.empty() && line.back() == '\r') {
                    line.pop_back();
                }

                const auto commentPos = FindCommentStart(line);
                if (commentPos != std::string::npos) {
                    line.erase(commentPos);
                }

                const auto trimmed = Trim(line);
                if (trimmed.empty()) {
                    continue;
                }

                if (trimmed.front() == '[' && trimmed.back() == ']') {
                    section = Lower(Trim(std::string_view(trimmed).substr(1, trimmed.size() - 2)));
                    continue;
                }

                const auto equals = trimmed.find('=');
                if (equals == std::string::npos) {
                    continue;
                }

                const auto key = Lower(Trim(std::string_view(trimmed).substr(0, equals)));
                const auto value = Trim(std::string_view(trimmed).substr(equals + 1));
                map[section][key] = value;
            }

            return map;
        }

        [[nodiscard]] const std::string* Find(const SectionMap& a_map, std::string_view a_section, std::string_view a_key)
        {
            const auto sectionIt = a_map.find(std::string(a_section));
            if (sectionIt == a_map.end()) {
                return nullptr;
            }

            const auto keyIt = sectionIt->second.find(std::string(a_key));
            return keyIt == sectionIt->second.end() ? nullptr : std::addressof(keyIt->second);
        }
    }

    SettingsLoadResult LoadSettings(const std::filesystem::path& a_path)
    {
        std::ifstream input(a_path);
        if (!input) {
            return {};
        }

        std::ostringstream buffer;
        buffer << input.rdbuf();
        auto result = ParseSettings(buffer.str());
        result.loadedFromFile = true;
        return result;
    }

    SettingsLoadResult ParseSettings(std::string_view a_iniText)
    {
        SettingsLoadResult result;
        auto& settings = result.settings;
        auto& warnings = result.warnings;
        const auto map = ParseIniMap(a_iniText);

        if (const auto* value = Find(map, "general", "benabled")) {
            settings.enabled = ParseBool(*value, settings.enabled, warnings, "bEnabled");
        }
        if (const auto* value = Find(map, "general", "bfadeondialogue")) {
            settings.fadeOnDialogue = ParseBool(*value, settings.fadeOnDialogue, warnings, "bFadeOnDialogue");
        }
        if (const auto* value = Find(map, "general", "brestoreondialogueend")) {
            settings.restoreOnDialogueEnd = ParseBool(*value, settings.restoreOnDialogueEnd, warnings, "bRestoreOnDialogueEnd");
        }
        if (const auto* value = Find(map, "general", "bdebuglogging")) {
            settings.debugLogging = ParseBool(*value, settings.debugLogging, warnings, "bDebugLogging");
        }

        if (const auto* value = Find(map, "animation", "ffadeoutseconds")) {
            settings.fadeOutSeconds = ParseFloat(*value, settings.fadeOutSeconds, 0.0F, 10.0F, warnings, "fFadeOutSeconds");
        }
        if (const auto* value = Find(map, "animation", "ffadeinseconds")) {
            settings.fadeInSeconds = ParseFloat(*value, settings.fadeInSeconds, 0.0F, 10.0F, warnings, "fFadeInSeconds");
        }
        if (const auto* value = Find(map, "animation", "fhiddenalpha")) {
            settings.hiddenAlpha = ParseFloat(*value, settings.hiddenAlpha, 0.0F, 100.0F, warnings, "fHiddenAlpha");
        }
        if (const auto* value = Find(map, "animation", "fvisiblealpha")) {
            settings.visibleAlpha = ParseFloat(*value, settings.visibleAlpha, 0.0F, 100.0F, warnings, "fVisibleAlpha");
        }
        if (settings.hiddenAlpha > settings.visibleAlpha) {
            warnings.emplace_back("fHiddenAlpha was greater than fVisibleAlpha; values were swapped");
            std::swap(settings.hiddenAlpha, settings.visibleAlpha);
        }
        if (const auto* value = Find(map, "animation", "seasing")) {
            settings.easing = EasingFromString(*value, std::addressof(warnings));
        }

        if (const auto* value = Find(map, "compatibility", "brespectexternalhudchanges")) {
            settings.respectExternalHudChanges = ParseBool(*value, settings.respectExternalHudChanges, warnings, "bRespectExternalHudChanges");
        }
        if (const auto* value = Find(map, "compatibility", "brestoreonlyifchangedbyplugin")) {
            settings.restoreOnlyIfChangedByPlugin = ParseBool(*value, settings.restoreOnlyIfChangedByPlugin, warnings, "bRestoreOnlyIfChangedByPlugin");
        }
        if (const auto* value = Find(map, "compatibility", "bretryifhudmenumissing")) {
            settings.retryIfHUDMenuMissing = ParseBool(*value, settings.retryIfHUDMenuMissing, warnings, "bRetryIfHUDMenuMissing");
        }
        if (const auto* value = Find(map, "compatibility", "bdisableinmenus")) {
            settings.disableInMenus = ParseBool(*value, settings.disableInMenus, warnings, "bDisableInMenus");
        }

        if (const auto* value = Find(map, "targets", "stargetmode")) {
            const auto mode = Lower(Unquote(*value));
            if (mode != "fullhud") {
                warnings.emplace_back("sTargetMode is unsupported; FullHUD used");
            }
            settings.targetMode = TargetMode::kFullHUD;
        }
        if (const auto* value = Find(map, "targets", "sfullhudpaths")) {
            auto paths = SplitCsv(*value);
            if (!paths.empty()) {
                settings.fullHudPaths = std::move(paths);
            } else {
                warnings.emplace_back("sFullHUDPaths was empty; defaults used");
            }
        }

        return result;
    }

    const char* ToString(Easing a_easing) noexcept
    {
        switch (a_easing) {
        case Easing::kSmoothstep:
            return "smoothstep";
        case Easing::kEaseOutCubic:
            return "easeOutCubic";
        case Easing::kLinear:
            return "linear";
        default:
            return "smoothstep";
        }
    }

    Easing EasingFromString(std::string_view a_value, std::vector<std::string>* a_warnings)
    {
        const auto easing = Lower(Unquote(a_value));
        if (easing == "smoothstep") {
            return Easing::kSmoothstep;
        }
        if (easing == "easeoutcubic" || easing == "ease_out_cubic") {
            return Easing::kEaseOutCubic;
        }
        if (easing == "ease-out-cubic" || easing == "ease out cubic") {
            return Easing::kEaseOutCubic;
        }
        if (easing == "linear") {
            return Easing::kLinear;
        }

        if (a_warnings) {
            a_warnings->emplace_back("sEasing is unsupported; smoothstep used");
        }
        return Easing::kSmoothstep;
    }
}

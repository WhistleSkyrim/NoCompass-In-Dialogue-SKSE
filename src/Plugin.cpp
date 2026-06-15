#include "PCH.h"

#include "Plugin.h"

#include "DialogueMenuWatcher.h"
#include "HudFader.h"
#include "Logger.h"
#include "ScaleformHudTarget.h"

namespace DialogueHUDFade::Plugin
{
    namespace
    {
        Settings g_settings{};
        HudFader g_fader{ g_settings };
        std::unique_ptr<ScaleformHudTarget> g_target;
        bool g_hookInstalled{ false };

        using AdvanceMovie_t = void(RE::HUDMenu*, float, std::uint32_t);
        REL::Relocation<AdvanceMovie_t> g_advanceMovie;

        [[nodiscard]] std::filesystem::path PluginDirectory()
        {
            HMODULE module = nullptr;
            const auto flags = GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT;
            if (::GetModuleHandleExW(flags, reinterpret_cast<LPCWSTR>(&PluginDirectory), std::addressof(module)) != 0) {
                std::wstring buffer(MAX_PATH, L'\0');
                while (buffer.size() <= 32768) {
                    const auto size = ::GetModuleFileNameW(module, buffer.data(), static_cast<DWORD>(buffer.size()));
                    if (size == 0) {
                        break;
                    }

                    if (size < buffer.size()) {
                        buffer.resize(size);
                        return std::filesystem::path(buffer).parent_path();
                    }

                    buffer.resize(buffer.size() * 2);
                }
            }

            return std::filesystem::current_path() / "Data" / "SKSE" / "Plugins";
        }

        [[nodiscard]] bool IsBlockingMenuOpen(RE::UI& a_ui)
        {
            if (a_ui.GameIsPaused() || a_ui.IsApplicationMenuOpen() || a_ui.IsItemMenuOpen() || a_ui.IsModalMenuOpen()) {
                return true;
            }

            constexpr std::array<std::string_view, 22> blockingMenus{
                RE::BarterMenu::MENU_NAME,
                RE::BookMenu::MENU_NAME,
                RE::Console::MENU_NAME,
                RE::ConsoleNativeUIMenu::MENU_NAME,
                RE::ContainerMenu::MENU_NAME,
                RE::CraftingMenu::MENU_NAME,
                RE::CreationClubMenu::MENU_NAME,
                RE::FavoritesMenu::MENU_NAME,
                RE::GiftMenu::MENU_NAME,
                RE::InventoryMenu::MENU_NAME,
                RE::JournalMenu::MENU_NAME,
                RE::LevelUpMenu::MENU_NAME,
                RE::LockpickingMenu::MENU_NAME,
                RE::MagicMenu::MENU_NAME,
                RE::MapMenu::MENU_NAME,
                RE::MessageBoxMenu::MENU_NAME,
                RE::ModManagerMenu::MENU_NAME,
                RE::RaceSexMenu::MENU_NAME,
                RE::SleepWaitMenu::MENU_NAME,
                RE::StatsMenu::MENU_NAME,
                RE::TrainingMenu::MENU_NAME,
                RE::TweenMenu::MENU_NAME,
            };

            for (const auto menuName : blockingMenus) {
                if (a_ui.IsMenuOpen(menuName)) {
                    return true;
                }
            }

            return false;
        }

        [[nodiscard]] bool ShouldSuspendForOpenMenu()
        {
            if (!g_settings.disableInMenus) {
                return false;
            }

            const auto ui = RE::UI::GetSingleton();
            return ui && IsBlockingMenuOpen(*ui);
        }

        [[nodiscard]] const char* RuntimeName() noexcept
        {
            if (REL::Module::IsVR()) {
                return "VR";
            }
            if (REL::Module::IsAE()) {
                return "AE/GOG";
            }
            if (REL::Module::IsSE()) {
                return "SE";
            }
            return "Unknown";
        }

        void AdvanceMovie(RE::HUDMenu* a_menu, float a_interval, std::uint32_t a_currentTime)
        {
            g_advanceMovie(a_menu, a_interval, a_currentTime);
            OnFrame(a_interval);
        }

        bool InstallHudAdvanceHook()
        {
            if (g_hookInstalled) {
                return true;
            }

            REL::Relocation<std::uintptr_t> vtable{ RE::VTABLE_HUDMenu[0] };
            g_advanceMovie = vtable.write_vfunc(0x05, AdvanceMovie);
            g_hookInstalled = true;
            spdlog::info("Installed HUDMenu::AdvanceMovie hook through Address Library VTABLE relocation");
            return true;
        }

        void LoadConfiguration()
        {
            const auto iniPath = PluginDirectory() / "DialogueHUDFade.ini";
            auto result = LoadSettings(iniPath);
            g_settings = std::move(result.settings);
            g_fader.Configure(g_settings);
            g_target = std::make_unique<ScaleformHudTarget>(g_settings);

            if (result.loadedFromFile) {
                spdlog::info("Loaded INI: {}", iniPath.string());
            } else {
                spdlog::info("INI not found; using defaults");
            }

            for (const auto& warning : result.warnings) {
                spdlog::warn("INI: {}", warning);
            }
        }

        [[nodiscard]] bool RegisterMessaging()
        {
            const auto messaging = SKSE::GetMessagingInterface();
            if (!messaging || !messaging->RegisterListener(OnSKSEMessage)) {
                spdlog::error("Could not register SKSE messaging listener");
                return false;
            }

            spdlog::info("Registered SKSE messaging listener");
            return true;
        }
    }

    bool Initialize(const SKSE::LoadInterface* a_skse)
    {
        const auto runtimeVersion = a_skse ? a_skse->RuntimeVersion().string(".") : std::string("unknown");
        const auto skseVersion = a_skse ? a_skse->SKSEVersion() : 0U;

        spdlog::info("{} version {}", NAME, VERSION);
        spdlog::info("Runtime: {} {}", RuntimeName(), runtimeVersion);
        spdlog::info("SKSE version: {}", skseVersion);
        spdlog::info("Address Library: required by CommonLibSSE-NG plugin metadata");

        if (REL::Module::GetRuntime() == REL::Module::Runtime::Unknown) {
            spdlog::critical("Unsupported runtime: unknown Skyrim executable");
            return false;
        }

        LoadConfiguration();
        Logger::SetDebugLogging(g_settings.debugLogging);
        if (!RegisterMessaging()) {
            return false;
        }

        try {
            InstallHudAdvanceHook();
        } catch (const std::exception& ex) {
            spdlog::critical("Failed to install HUD advance hook: {}", ex.what());
            return false;
        }

        return true;
    }

    void OnSKSEMessage(SKSE::MessagingInterface::Message* a_message)
    {
        if (!a_message) {
            return;
        }

        switch (a_message->type) {
        case SKSE::MessagingInterface::kDataLoaded:
            DialogueMenuWatcher::GetSingleton().Register();
            break;
        case SKSE::MessagingInterface::kPreLoadGame:
        case SKSE::MessagingInterface::kPostLoadGame:
        case SKSE::MessagingInterface::kNewGame:
            OnMenuReset();
            break;
        default:
            break;
        }
    }

    void OnDialogueOpen()
    {
        if (!g_target) {
            return;
        }

        if (ShouldSuspendForOpenMenu()) {
            if (g_settings.debugLogging) {
                spdlog::debug("Dialogue Menu opened while another blocking menu is active; fade skipped");
            }
            g_fader.OnMenuReset(*g_target);
            return;
        }

        if (g_settings.debugLogging) {
            spdlog::debug("Dialogue Menu opened");
        }

        g_fader.OnDialogueOpen(*g_target);
    }

    void OnDialogueClose()
    {
        if (!g_target) {
            return;
        }

        if (ShouldSuspendForOpenMenu()) {
            g_fader.OnMenuReset(*g_target);
            return;
        }

        if (g_settings.debugLogging) {
            spdlog::debug("Dialogue Menu closed");
        }

        g_fader.OnDialogueClose(*g_target);
    }

    void OnMenuReset()
    {
        if (!g_target) {
            return;
        }

        g_fader.OnMenuReset(*g_target);
    }

    void OnFrame(float a_deltaSeconds)
    {
        if (!g_target || !g_settings.enabled) {
            return;
        }

        if (ShouldSuspendForOpenMenu()) {
            if (g_fader.IsAnimating() || g_fader.HasCapturedState()) {
                g_fader.OnMenuReset(*g_target);
            }
            return;
        }

        if (!g_fader.IsAnimating()) {
            return;
        }

        g_fader.Tick(a_deltaSeconds, *g_target);
    }

    const Settings& GetSettings()
    {
        return g_settings;
    }
}

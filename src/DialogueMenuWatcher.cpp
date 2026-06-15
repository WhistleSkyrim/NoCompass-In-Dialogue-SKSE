#include "PCH.h"

#include "DialogueMenuWatcher.h"
#include "Plugin.h"

namespace DialogueHUDFade
{
    namespace
    {
        [[nodiscard]] bool IsMenu(const RE::BSFixedString& a_name, std::string_view a_expected)
        {
            return a_name == a_expected;
        }

        [[nodiscard]] bool IsDialogueMenu(const RE::BSFixedString& a_name)
        {
            return IsMenu(a_name, RE::DialogueMenu::MENU_NAME) || IsMenu(a_name, "Dialogue Menu");
        }

        [[nodiscard]] bool IsResetMenu(const RE::BSFixedString& a_name)
        {
            return IsMenu(a_name, RE::MainMenu::MENU_NAME) || IsMenu(a_name, "Main Menu") ||
                   IsMenu(a_name, RE::LoadingMenu::MENU_NAME) || IsMenu(a_name, "Loading Menu");
        }

        [[nodiscard]] bool IsBlockingMenu(const RE::BSFixedString& a_name)
        {
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
                if (IsMenu(a_name, menuName)) {
                    return true;
                }
            }

            return false;
        }
    }

    DialogueMenuWatcher& DialogueMenuWatcher::GetSingleton()
    {
        static DialogueMenuWatcher singleton;
        return singleton;
    }

    void DialogueMenuWatcher::Register()
    {
        if (registered_) {
            return;
        }

        const auto ui = RE::UI::GetSingleton();
        if (!ui) {
            spdlog::error("Could not register menu event sink: RE::UI is unavailable");
            return;
        }

        ui->AddEventSink<RE::MenuOpenCloseEvent>(this);
        registered_ = true;
        spdlog::info("Registered MenuOpenCloseEvent sink");
    }

    void DialogueMenuWatcher::Unregister()
    {
        if (!registered_) {
            return;
        }

        const auto ui = RE::UI::GetSingleton();
        if (ui) {
            ui->RemoveEventSink<RE::MenuOpenCloseEvent>(this);
        }

        registered_ = false;
    }

    RE::BSEventNotifyControl DialogueMenuWatcher::ProcessEvent(const RE::MenuOpenCloseEvent* a_event, RE::BSTEventSource<RE::MenuOpenCloseEvent>*)
    {
        if (!a_event) {
            return RE::BSEventNotifyControl::kContinue;
        }

        if (IsDialogueMenu(a_event->menuName)) {
            if (a_event->opening) {
                Plugin::OnDialogueOpen();
            } else {
                Plugin::OnDialogueClose();
            }
            return RE::BSEventNotifyControl::kContinue;
        }

        if (a_event->opening && IsResetMenu(a_event->menuName)) {
            Plugin::OnMenuReset();
            return RE::BSEventNotifyControl::kContinue;
        }

        if (a_event->opening && Plugin::GetSettings().disableInMenus && IsBlockingMenu(a_event->menuName)) {
            Plugin::OnMenuReset();
        }

        return RE::BSEventNotifyControl::kContinue;
    }
}

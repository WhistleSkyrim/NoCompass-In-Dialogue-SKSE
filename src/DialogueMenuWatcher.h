#pragma once

namespace DialogueHUDFade
{
    class DialogueMenuWatcher final : public RE::BSTEventSink<RE::MenuOpenCloseEvent>
    {
    public:
        static DialogueMenuWatcher& GetSingleton();

        void Register();
        void Unregister();

        RE::BSEventNotifyControl ProcessEvent(const RE::MenuOpenCloseEvent* a_event, RE::BSTEventSource<RE::MenuOpenCloseEvent>* a_eventSource) override;

    private:
        DialogueMenuWatcher() = default;

        bool registered_{ false };
    };
}

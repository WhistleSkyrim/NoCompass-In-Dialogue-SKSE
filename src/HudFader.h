#pragma once

#include "Settings.h"

#include <optional>

namespace DialogueHUDFade
{
    enum class FaderState
    {
        kIdleVisible,
        kFadingOut,
        kHidden,
        kFadingIn
    };

    struct HudState
    {
        std::optional<float> alpha;
        std::optional<bool> visible;
        bool available{ false };
    };

    class IHudTarget
    {
    public:
        virtual ~IHudTarget() = default;

        [[nodiscard]] virtual HudState Read() = 0;
        virtual bool ApplyAlpha(float a_alpha) = 0;
    };

    class HudFader
    {
    public:
        explicit HudFader(Settings a_settings = {});

        void Configure(Settings a_settings);

        void OnDialogueOpen(IHudTarget& a_target);
        void OnDialogueClose(IHudTarget& a_target);
        void OnMenuReset(IHudTarget& a_target);
        void Tick(float a_deltaSeconds, IHudTarget& a_target);

        [[nodiscard]] FaderState GetState() const noexcept;
        [[nodiscard]] float GetCurrentAlpha() const noexcept;
        [[nodiscard]] bool IsAnimating() const noexcept;
        [[nodiscard]] bool HasCapturedState() const noexcept;
        [[nodiscard]] bool HasExternalOverride() const noexcept;

    private:
        void BeginFade(float a_from, float a_to, float a_duration, FaderState a_fadingState, FaderState a_finishedState, IHudTarget& a_target);
        [[nodiscard]] bool FinishAtTarget(IHudTarget& a_target);
        [[nodiscard]] bool Apply(float a_alpha, IHudTarget& a_target);
        [[nodiscard]] bool CaptureIfNeeded(IHudTarget& a_target);
        void ClearCapture() noexcept;
        void ClearCaptureForExternalOverride() noexcept;
        [[nodiscard]] float Ease(float a_t) const noexcept;
        [[nodiscard]] float RestoreAlpha() const noexcept;
        [[nodiscard]] bool DetectExternalChange(IHudTarget& a_target);

        Settings settings_{};
        FaderState state_{ FaderState::kIdleVisible };
        FaderState finishedState_{ FaderState::kIdleVisible };

        bool dialogueOpen_{ false };
        bool captured_{ false };
        bool changedByPlugin_{ false };
        bool externalOverride_{ false };
        bool hasLastAppliedAlpha_{ false };

        std::optional<float> capturedAlpha_{};
        std::optional<bool> capturedVisible_{};

        float currentAlpha_{ 100.0F };
        float startAlpha_{ 100.0F };
        float targetAlpha_{ 100.0F };
        float elapsedSeconds_{ 0.0F };
        float durationSeconds_{ 0.0F };
        float lastAppliedAlpha_{ 100.0F };
    };

    [[nodiscard]] float ClampAlpha(float a_alpha) noexcept;
    [[nodiscard]] const char* ToString(FaderState a_state) noexcept;
}

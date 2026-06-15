#include "HudFader.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace DialogueHUDFade
{
    namespace
    {
        constexpr float kExternalChangeTolerance = 1.0F;
        constexpr float kMaxAnimationDeltaSeconds = 0.05F;
        constexpr float kMaxAnimationSeconds = 10.0F;

        [[nodiscard]] bool NearlyEqual(float a_lhs, float a_rhs, float a_tolerance = kExternalChangeTolerance) noexcept
        {
            return std::fabs(a_lhs - a_rhs) <= a_tolerance;
        }

        [[nodiscard]] float ClampDuration(float a_seconds) noexcept
        {
            if (!std::isfinite(a_seconds)) {
                return 0.0F;
            }

            return std::clamp(a_seconds, 0.0F, kMaxAnimationSeconds);
        }

        [[nodiscard]] float ClampDelta(float a_seconds) noexcept
        {
            if (!std::isfinite(a_seconds)) {
                return 0.0F;
            }

            return std::clamp(a_seconds, 0.0F, kMaxAnimationDeltaSeconds);
        }

        [[nodiscard]] Settings NormalizeSettings(Settings a_settings)
        {
            a_settings.fadeOutSeconds = ClampDuration(a_settings.fadeOutSeconds);
            a_settings.fadeInSeconds = ClampDuration(a_settings.fadeInSeconds);
            a_settings.hiddenAlpha = ClampAlpha(a_settings.hiddenAlpha);
            a_settings.visibleAlpha = ClampAlpha(a_settings.visibleAlpha);
            if (a_settings.hiddenAlpha > a_settings.visibleAlpha) {
                std::swap(a_settings.hiddenAlpha, a_settings.visibleAlpha);
            }
            return a_settings;
        }
    }

    HudFader::HudFader(Settings a_settings) :
        settings_(NormalizeSettings(std::move(a_settings))),
        currentAlpha_(settings_.visibleAlpha),
        startAlpha_(settings_.visibleAlpha),
        targetAlpha_(settings_.visibleAlpha),
        lastAppliedAlpha_(settings_.visibleAlpha)
    {}

    void HudFader::Configure(Settings a_settings)
    {
        settings_ = NormalizeSettings(std::move(a_settings));
        currentAlpha_ = ClampAlpha(currentAlpha_);
        startAlpha_ = ClampAlpha(startAlpha_);
        targetAlpha_ = ClampAlpha(targetAlpha_);
        lastAppliedAlpha_ = ClampAlpha(lastAppliedAlpha_);
    }

    void HudFader::OnDialogueOpen(IHudTarget& a_target)
    {
        const auto wasDialogueOpen = dialogueOpen_;
        dialogueOpen_ = true;

        if (wasDialogueOpen && externalOverride_) {
            return;
        }

        externalOverride_ = false;

        if (!settings_.enabled || !settings_.fadeOnDialogue) {
            return;
        }

        (void)CaptureIfNeeded(a_target);

        const auto read = a_target.Read();
        const auto from = read.alpha.value_or(currentAlpha_);
        BeginFade(from, settings_.hiddenAlpha, settings_.fadeOutSeconds, FaderState::kFadingOut, FaderState::kHidden, a_target);
    }

    void HudFader::OnDialogueClose(IHudTarget& a_target)
    {
        dialogueOpen_ = false;

        if (!settings_.enabled || !settings_.restoreOnDialogueEnd) {
            ClearCapture();
            return;
        }

        if (settings_.respectExternalHudChanges && (externalOverride_ || DetectExternalChange(a_target))) {
            ClearCapture();
            state_ = FaderState::kIdleVisible;
            finishedState_ = FaderState::kIdleVisible;
            elapsedSeconds_ = 0.0F;
            durationSeconds_ = 0.0F;
            return;
        }

        if (settings_.restoreOnlyIfChangedByPlugin && !changedByPlugin_) {
            ClearCapture();
            state_ = FaderState::kIdleVisible;
            finishedState_ = FaderState::kIdleVisible;
            elapsedSeconds_ = 0.0F;
            durationSeconds_ = 0.0F;
            return;
        }

        const auto read = a_target.Read();
        const auto from = read.alpha.value_or(currentAlpha_);
        BeginFade(from, RestoreAlpha(), settings_.fadeInSeconds, FaderState::kFadingIn, FaderState::kIdleVisible, a_target);
    }

    void HudFader::OnMenuReset(IHudTarget& a_target)
    {
        const auto externalChange = settings_.respectExternalHudChanges && (externalOverride_ || DetectExternalChange(a_target));

        if (changedByPlugin_ && !externalChange) {
            const auto restoreAlpha = RestoreAlpha();
            (void)Apply(restoreAlpha, a_target);
            currentAlpha_ = restoreAlpha;
        }

        ClearCapture();
        dialogueOpen_ = false;
        state_ = FaderState::kIdleVisible;
        finishedState_ = FaderState::kIdleVisible;
        elapsedSeconds_ = 0.0F;
        durationSeconds_ = 0.0F;
    }

    void HudFader::Tick(float a_deltaSeconds, IHudTarget& a_target)
    {
        if (!IsAnimating()) {
            return;
        }

        if (state_ == FaderState::kFadingOut && !captured_) {
            if (!CaptureIfNeeded(a_target) && settings_.retryIfHUDMenuMissing) {
                return;
            }

            startAlpha_ = currentAlpha_;
            elapsedSeconds_ = 0.0F;
        }

        if (settings_.respectExternalHudChanges && DetectExternalChange(a_target)) {
            state_ = dialogueOpen_ ? FaderState::kHidden : FaderState::kIdleVisible;
            finishedState_ = state_;
            elapsedSeconds_ = 0.0F;
            durationSeconds_ = 0.0F;
            ClearCaptureForExternalOverride();
            return;
        }

        if (durationSeconds_ <= 0.0F) {
            (void)FinishAtTarget(a_target);
            return;
        }

        const auto deltaSeconds = ClampDelta(a_deltaSeconds);
        const auto nextElapsedSeconds = std::min(durationSeconds_, elapsedSeconds_ + deltaSeconds);
        const auto t = nextElapsedSeconds / durationSeconds_;
        const auto eased = Ease(std::clamp(t, 0.0F, 1.0F));
        const auto nextAlpha = std::lerp(startAlpha_, targetAlpha_, eased);
        if (!Apply(nextAlpha, a_target) && settings_.retryIfHUDMenuMissing) {
            return;
        }

        elapsedSeconds_ = nextElapsedSeconds;
        currentAlpha_ = nextAlpha;

        if (elapsedSeconds_ >= durationSeconds_) {
            (void)FinishAtTarget(a_target);
        }
    }

    FaderState HudFader::GetState() const noexcept
    {
        return state_;
    }

    float HudFader::GetCurrentAlpha() const noexcept
    {
        return currentAlpha_;
    }

    bool HudFader::IsAnimating() const noexcept
    {
        return state_ == FaderState::kFadingOut || state_ == FaderState::kFadingIn;
    }

    bool HudFader::HasCapturedState() const noexcept
    {
        return captured_;
    }

    bool HudFader::HasExternalOverride() const noexcept
    {
        return externalOverride_;
    }

    void HudFader::BeginFade(float a_from, float a_to, float a_duration, FaderState a_fadingState, FaderState a_finishedState, IHudTarget& a_target)
    {
        startAlpha_ = ClampAlpha(a_from);
        currentAlpha_ = startAlpha_;
        targetAlpha_ = ClampAlpha(a_to);
        durationSeconds_ = ClampDuration(a_duration);
        elapsedSeconds_ = 0.0F;
        state_ = a_fadingState;
        finishedState_ = a_finishedState;

        if (durationSeconds_ <= 0.0F || NearlyEqual(startAlpha_, targetAlpha_, 0.001F)) {
            (void)FinishAtTarget(a_target);
        }
    }

    bool HudFader::FinishAtTarget(IHudTarget& a_target)
    {
        if (!Apply(targetAlpha_, a_target) && settings_.retryIfHUDMenuMissing) {
            return false;
        }

        currentAlpha_ = targetAlpha_;
        state_ = finishedState_;

        if (state_ == FaderState::kIdleVisible) {
            ClearCapture();
        }

        return true;
    }

    bool HudFader::Apply(float a_alpha, IHudTarget& a_target)
    {
        const auto clamped = ClampAlpha(a_alpha);
        if (a_target.ApplyAlpha(clamped)) {
            changedByPlugin_ = true;
            hasLastAppliedAlpha_ = true;
            lastAppliedAlpha_ = clamped;
            return true;
        }

        return false;
    }

    bool HudFader::CaptureIfNeeded(IHudTarget& a_target)
    {
        if (captured_) {
            return true;
        }

        const auto read = a_target.Read();
        capturedAlpha_ = read.alpha;
        capturedVisible_ = read.visible;
        if (capturedAlpha_) {
            currentAlpha_ = ClampAlpha(*capturedAlpha_);
        }
        captured_ = read.available || capturedAlpha_.has_value() || capturedVisible_.has_value();
        changedByPlugin_ = false;
        hasLastAppliedAlpha_ = false;
        return captured_;
    }

    void HudFader::ClearCapture() noexcept
    {
        captured_ = false;
        capturedAlpha_.reset();
        capturedVisible_.reset();
        changedByPlugin_ = false;
        externalOverride_ = false;
        hasLastAppliedAlpha_ = false;
    }

    void HudFader::ClearCaptureForExternalOverride() noexcept
    {
        ClearCapture();
        externalOverride_ = true;
    }

    float HudFader::Ease(float a_t) const noexcept
    {
        const auto t = std::clamp(a_t, 0.0F, 1.0F);
        switch (settings_.easing) {
        case Easing::kLinear:
            return t;
        case Easing::kEaseOutCubic: {
            const auto inv = 1.0F - t;
            return 1.0F - inv * inv * inv;
        }
        case Easing::kSmoothstep:
        default:
            return t * t * (3.0F - 2.0F * t);
        }
    }

    float HudFader::RestoreAlpha() const noexcept
    {
        return ClampAlpha(capturedAlpha_.value_or(settings_.visibleAlpha));
    }

    bool HudFader::DetectExternalChange(IHudTarget& a_target)
    {
        if (!hasLastAppliedAlpha_) {
            return false;
        }

        const auto read = a_target.Read();
        if (!read.alpha) {
            return false;
        }

        if (NearlyEqual(*read.alpha, lastAppliedAlpha_)) {
            return false;
        }

        currentAlpha_ = ClampAlpha(*read.alpha);
        return true;
    }

    float ClampAlpha(float a_alpha) noexcept
    {
        if (!std::isfinite(a_alpha)) {
            return 0.0F;
        }

        return std::clamp(a_alpha, 0.0F, 100.0F);
    }

    const char* ToString(FaderState a_state) noexcept
    {
        switch (a_state) {
        case FaderState::kIdleVisible:
            return "IdleVisible";
        case FaderState::kFadingOut:
            return "FadingOut";
        case FaderState::kHidden:
            return "Hidden";
        case FaderState::kFadingIn:
            return "FadingIn";
        default:
            return "Unknown";
        }
    }
}

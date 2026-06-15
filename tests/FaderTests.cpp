#include "TestHarness.h"

#include <limits>

namespace DialogueHUDFade::Tests
{
    void FaderReachesHiddenTarget()
    {
        Settings settings;
        settings.easing = Easing::kLinear;
        settings.fadeOutSeconds = 0.20F;
        MockHudTarget hud;
        HudFader fader(settings);

        fader.OnDialogueOpen(hud);
        DHF_REQUIRE(fader.GetState() == FaderState::kFadingOut);
        fader.Tick(0.05F, hud);
        fader.Tick(0.05F, hud);
        DHF_REQUIRE_NEAR(*hud.alpha, 50.0F, 0.1F);
        fader.Tick(0.05F, hud);
        fader.Tick(0.05F, hud);
        DHF_REQUIRE(fader.GetState() == FaderState::kHidden);
        DHF_REQUIRE_NEAR(*hud.alpha, settings.hiddenAlpha, 0.01F);
    }

    void FaderReversesMidAnimation()
    {
        Settings settings;
        settings.easing = Easing::kLinear;
        settings.fadeOutSeconds = 0.20F;
        settings.fadeInSeconds = 0.20F;
        MockHudTarget hud;
        HudFader fader(settings);

        fader.OnDialogueOpen(hud);
        fader.Tick(0.05F, hud);
        fader.Tick(0.05F, hud);
        DHF_REQUIRE_NEAR(*hud.alpha, 50.0F, 0.1F);

        fader.OnDialogueClose(hud);
        DHF_REQUIRE(fader.GetState() == FaderState::kFadingIn);
        fader.Tick(0.05F, hud);
        fader.Tick(0.05F, hud);
        DHF_REQUIRE_NEAR(*hud.alpha, 75.0F, 0.1F);
        fader.Tick(0.05F, hud);
        fader.Tick(0.05F, hud);
        DHF_REQUIRE(fader.GetState() == FaderState::kIdleVisible);
        DHF_REQUIRE_NEAR(*hud.alpha, 100.0F, 0.01F);
    }

    void ZeroDurationFadeIsInstant()
    {
        Settings settings;
        settings.fadeOutSeconds = 0.0F;
        MockHudTarget hud;
        HudFader fader(settings);

        fader.OnDialogueOpen(hud);
        DHF_REQUIRE(fader.GetState() == FaderState::kHidden);
        DHF_REQUIRE_NEAR(*hud.alpha, 0.0F, 0.01F);
    }

    void LargeFrameDeltaDoesNotSkipFade()
    {
        Settings settings;
        settings.easing = Easing::kLinear;
        settings.fadeOutSeconds = 0.20F;
        MockHudTarget hud;
        HudFader fader(settings);

        fader.OnDialogueOpen(hud);
        fader.Tick(1.0F, hud);

        DHF_REQUIRE(fader.GetState() == FaderState::kFadingOut);
        DHF_REQUIRE_NEAR(*hud.alpha, 75.0F, 0.1F);
    }

    void NonFiniteAlphaIsClampedSafely()
    {
        DHF_REQUIRE_NEAR(ClampAlpha(std::numeric_limits<float>::quiet_NaN()), 0.0F, 0.01F);
        DHF_REQUIRE_NEAR(ClampAlpha(std::numeric_limits<float>::infinity()), 0.0F, 0.01F);
    }

    void NonFiniteDurationFallsBackToInstantFade()
    {
        Settings settings;
        settings.fadeOutSeconds = std::numeric_limits<float>::infinity();
        MockHudTarget hud;
        HudFader fader(settings);

        fader.OnDialogueOpen(hud);

        DHF_REQUIRE(fader.GetState() == FaderState::kHidden);
        DHF_REQUIRE_NEAR(*hud.alpha, settings.hiddenAlpha, 0.01F);
    }

    void InstantFadeRetriesWhenHudBecomesAvailable()
    {
        Settings settings;
        settings.fadeOutSeconds = 0.0F;
        settings.retryIfHUDMenuMissing = true;
        MockHudTarget hud;
        hud.alpha.reset();
        hud.visible.reset();
        hud.available = false;
        HudFader fader(settings);

        fader.OnDialogueOpen(hud);
        DHF_REQUIRE(fader.GetState() == FaderState::kFadingOut);
        DHF_REQUIRE(hud.appliedValues.empty());

        hud.available = true;
        hud.alpha = 100.0F;
        hud.visible = true;
        fader.Tick(0.0F, hud);

        DHF_REQUIRE(fader.GetState() == FaderState::kHidden);
        DHF_REQUIRE_NEAR(*hud.alpha, 0.0F, 0.01F);
    }

    void MenuResetUpdatesCurrentAlpha()
    {
        Settings settings;
        settings.fadeOutSeconds = 0.0F;
        MockHudTarget hud;
        HudFader fader(settings);

        fader.OnDialogueOpen(hud);
        DHF_REQUIRE_NEAR(fader.GetCurrentAlpha(), 0.0F, 0.01F);

        fader.OnMenuReset(hud);

        DHF_REQUIRE(fader.GetState() == FaderState::kIdleVisible);
        DHF_REQUIRE_NEAR(*hud.alpha, 100.0F, 0.01F);
        DHF_REQUIRE_NEAR(fader.GetCurrentAlpha(), 100.0F, 0.01F);
    }
}

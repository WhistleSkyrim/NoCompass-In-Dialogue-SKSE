#include "TestHarness.h"

#include <iostream>
#include <utility>

namespace DialogueHUDFade::Tests
{
    void FaderReachesHiddenTarget();
    void FaderReversesMidAnimation();
    void ZeroDurationFadeIsInstant();
    void LargeFrameDeltaDoesNotSkipFade();
    void NonFiniteAlphaIsClampedSafely();
    void NonFiniteDurationFallsBackToInstantFade();
    void InstantFadeRetriesWhenHudBecomesAvailable();
    void MenuResetUpdatesCurrentAlpha();
    void InvalidIniValuesAreClamped();
    void IniPathsAreParsed();
    void NonFiniteIniNumbersUseDefaults();
    void IniCommentsAndQuotedValuesAreParsed();
    void QuotedScalarValuesAreParsed();
    void IniPathListKeepsQuotedCommas();
    void HiddenAlphaCannotExceedVisibleAlpha();

    void RestoreDoesNotForceAlreadyHiddenHudVisible()
    {
        Settings settings;
        settings.easing = Easing::kLinear;
        settings.fadeOutSeconds = 0.0F;
        settings.fadeInSeconds = 0.0F;
        MockHudTarget hud;
        hud.alpha = 0.0F;
        HudFader fader(settings);

        fader.OnDialogueOpen(hud);
        fader.OnDialogueClose(hud);

        DHF_REQUIRE(fader.GetState() == FaderState::kIdleVisible);
        DHF_REQUIRE_NEAR(*hud.alpha, 0.0F, 0.01F);
    }

    void DialogueSequenceTransitions()
    {
        Settings settings;
        settings.easing = Easing::kLinear;
        MockHudTarget hud;
        HudFader fader(settings);

        fader.OnDialogueOpen(hud);
        DHF_REQUIRE(fader.GetState() == FaderState::kFadingOut);
        for (int i = 0; i < 4; ++i) {
            fader.Tick(0.05F, hud);
        }
        DHF_REQUIRE(fader.GetState() == FaderState::kHidden);
        fader.OnDialogueClose(hud);
        DHF_REQUIRE(fader.GetState() == FaderState::kFadingIn);
        for (int i = 0; i < 4; ++i) {
            fader.Tick(0.05F, hud);
        }
        DHF_REQUIRE(fader.GetState() == FaderState::kIdleVisible);
    }

    void RepeatedOpenCloseEventsRemainStable()
    {
        Settings settings;
        settings.easing = Easing::kLinear;
        MockHudTarget hud;
        HudFader fader(settings);

        fader.OnDialogueOpen(hud);
        fader.OnDialogueOpen(hud);
        for (int i = 0; i < 4; ++i) {
            fader.Tick(0.05F, hud);
        }
        DHF_REQUIRE(fader.GetState() == FaderState::kHidden);

        fader.OnDialogueClose(hud);
        fader.OnDialogueClose(hud);
        for (int i = 0; i < 4; ++i) {
            fader.Tick(0.05F, hud);
        }
        DHF_REQUIRE(fader.GetState() == FaderState::kIdleVisible);
        DHF_REQUIRE_NEAR(*hud.alpha, 100.0F, 0.01F);
    }

    void MissingHudTargetRetriesFadeWhenAvailable()
    {
        Settings settings;
        settings.easing = Easing::kLinear;
        settings.fadeOutSeconds = 0.20F;
        settings.retryIfHUDMenuMissing = true;
        MockHudTarget hud;
        hud.alpha.reset();
        hud.visible.reset();
        hud.available = false;
        HudFader fader(settings);

        fader.OnDialogueOpen(hud);
        DHF_REQUIRE(fader.GetState() == FaderState::kFadingOut);
        fader.Tick(0.05F, hud);
        DHF_REQUIRE(fader.GetState() == FaderState::kFadingOut);
        DHF_REQUIRE(hud.appliedValues.empty());

        hud.available = true;
        hud.alpha = 100.0F;
        hud.visible = true;
        fader.Tick(0.05F, hud);

        DHF_REQUIRE(fader.GetState() == FaderState::kFadingOut);
        DHF_REQUIRE_NEAR(*hud.alpha, 75.0F, 0.1F);
    }

    void ExternalHudChangeIsRespected()
    {
        Settings settings;
        settings.easing = Easing::kLinear;
        settings.respectExternalHudChanges = true;
        MockHudTarget hud;
        HudFader fader(settings);

        fader.OnDialogueOpen(hud);
        fader.Tick(0.05F, hud);
        hud.alpha = 88.0F;
        fader.Tick(0.05F, hud);

        DHF_REQUIRE(fader.HasExternalOverride());
        DHF_REQUIRE_NEAR(*hud.alpha, 88.0F, 0.01F);
    }

    void ExternalOverrideDuringRestoreDoesNotPoisonNextDialogue()
    {
        Settings settings;
        settings.easing = Easing::kLinear;
        settings.fadeOutSeconds = 0.0F;
        settings.fadeInSeconds = 0.20F;
        settings.respectExternalHudChanges = true;
        MockHudTarget hud;
        HudFader fader(settings);

        fader.OnDialogueOpen(hud);
        DHF_REQUIRE(fader.GetState() == FaderState::kHidden);
        DHF_REQUIRE_NEAR(*hud.alpha, 0.0F, 0.01F);

        fader.OnDialogueClose(hud);
        fader.Tick(0.05F, hud);
        DHF_REQUIRE_NEAR(*hud.alpha, 25.0F, 0.1F);

        hud.alpha = 20.0F;
        fader.Tick(0.05F, hud);
        DHF_REQUIRE(fader.GetState() == FaderState::kIdleVisible);
        DHF_REQUIRE(fader.HasExternalOverride());
        DHF_REQUIRE(!fader.HasCapturedState());

        fader.OnDialogueOpen(hud);
        DHF_REQUIRE(fader.GetState() == FaderState::kHidden);
        DHF_REQUIRE_NEAR(*hud.alpha, 0.0F, 0.01F);

        fader.OnDialogueClose(hud);
        for (int i = 0; i < 4; ++i) {
            fader.Tick(0.05F, hud);
        }

        DHF_REQUIRE(fader.GetState() == FaderState::kIdleVisible);
        DHF_REQUIRE_NEAR(*hud.alpha, 20.0F, 0.01F);
    }

    void DuplicateOpenDoesNotClearExternalOverride()
    {
        Settings settings;
        settings.easing = Easing::kLinear;
        settings.fadeOutSeconds = 0.20F;
        settings.respectExternalHudChanges = true;
        MockHudTarget hud;
        HudFader fader(settings);

        fader.OnDialogueOpen(hud);
        fader.Tick(0.05F, hud);
        hud.alpha = 88.0F;
        fader.Tick(0.05F, hud);

        DHF_REQUIRE(fader.HasExternalOverride());
        DHF_REQUIRE(fader.GetState() == FaderState::kHidden);

        fader.OnDialogueOpen(hud);

        DHF_REQUIRE(fader.HasExternalOverride());
        DHF_REQUIRE(fader.GetState() == FaderState::kHidden);
        DHF_REQUIRE_NEAR(*hud.alpha, 88.0F, 0.01F);
    }

    void ExternalHudChangeWhileHiddenIsNotRestoredOver()
    {
        Settings settings;
        settings.easing = Easing::kLinear;
        settings.fadeOutSeconds = 0.0F;
        settings.fadeInSeconds = 0.0F;
        settings.respectExternalHudChanges = true;
        MockHudTarget hud;
        HudFader fader(settings);

        fader.OnDialogueOpen(hud);
        DHF_REQUIRE(fader.GetState() == FaderState::kHidden);
        DHF_REQUIRE_NEAR(*hud.alpha, 0.0F, 0.01F);

        hud.alpha = 42.0F;
        fader.OnDialogueClose(hud);

        DHF_REQUIRE(fader.GetState() == FaderState::kIdleVisible);
        DHF_REQUIRE_NEAR(*hud.alpha, 42.0F, 0.01F);
        DHF_REQUIRE(!fader.HasCapturedState());
    }

    void MenuResetDoesNotRestoreOverExternalHiddenChange()
    {
        Settings settings;
        settings.easing = Easing::kLinear;
        settings.fadeOutSeconds = 0.0F;
        settings.respectExternalHudChanges = true;
        MockHudTarget hud;
        HudFader fader(settings);

        fader.OnDialogueOpen(hud);
        DHF_REQUIRE(fader.GetState() == FaderState::kHidden);
        DHF_REQUIRE_NEAR(*hud.alpha, 0.0F, 0.01F);

        hud.alpha = 35.0F;
        fader.OnMenuReset(hud);

        DHF_REQUIRE(fader.GetState() == FaderState::kIdleVisible);
        DHF_REQUIRE_NEAR(*hud.alpha, 35.0F, 0.01F);
        DHF_REQUIRE(!fader.HasCapturedState());
    }
}

int main()
{
    using namespace DialogueHUDFade::Tests;

    const std::pair<const char*, void (*)()> tests[] = {
        { "FaderReachesHiddenTarget", FaderReachesHiddenTarget },
        { "FaderReversesMidAnimation", FaderReversesMidAnimation },
        { "ZeroDurationFadeIsInstant", ZeroDurationFadeIsInstant },
        { "LargeFrameDeltaDoesNotSkipFade", LargeFrameDeltaDoesNotSkipFade },
        { "NonFiniteAlphaIsClampedSafely", NonFiniteAlphaIsClampedSafely },
        { "NonFiniteDurationFallsBackToInstantFade", NonFiniteDurationFallsBackToInstantFade },
        { "InstantFadeRetriesWhenHudBecomesAvailable", InstantFadeRetriesWhenHudBecomesAvailable },
        { "MenuResetUpdatesCurrentAlpha", MenuResetUpdatesCurrentAlpha },
        { "InvalidIniValuesAreClamped", InvalidIniValuesAreClamped },
        { "IniPathsAreParsed", IniPathsAreParsed },
        { "NonFiniteIniNumbersUseDefaults", NonFiniteIniNumbersUseDefaults },
        { "IniCommentsAndQuotedValuesAreParsed", IniCommentsAndQuotedValuesAreParsed },
        { "QuotedScalarValuesAreParsed", QuotedScalarValuesAreParsed },
        { "IniPathListKeepsQuotedCommas", IniPathListKeepsQuotedCommas },
        { "HiddenAlphaCannotExceedVisibleAlpha", HiddenAlphaCannotExceedVisibleAlpha },
        { "RestoreDoesNotForceAlreadyHiddenHudVisible", RestoreDoesNotForceAlreadyHiddenHudVisible },
        { "DialogueSequenceTransitions", DialogueSequenceTransitions },
        { "RepeatedOpenCloseEventsRemainStable", RepeatedOpenCloseEventsRemainStable },
        { "MissingHudTargetRetriesFadeWhenAvailable", MissingHudTargetRetriesFadeWhenAvailable },
        { "ExternalHudChangeIsRespected", ExternalHudChangeIsRespected },
        { "ExternalOverrideDuringRestoreDoesNotPoisonNextDialogue", ExternalOverrideDuringRestoreDoesNotPoisonNextDialogue },
        { "DuplicateOpenDoesNotClearExternalOverride", DuplicateOpenDoesNotClearExternalOverride },
        { "ExternalHudChangeWhileHiddenIsNotRestoredOver", ExternalHudChangeWhileHiddenIsNotRestoredOver },
        { "MenuResetDoesNotRestoreOverExternalHiddenChange", MenuResetDoesNotRestoreOverExternalHiddenChange },
    };

    int failures = 0;
    for (const auto& [name, test] : tests) {
        try {
            test();
            std::cout << "[PASS] " << name << '\n';
        } catch (const TestFailure& failure) {
            ++failures;
            std::cerr << "[FAIL] " << name << ": " << failure.what() << '\n';
        }
    }

    return failures == 0 ? 0 : 1;
}

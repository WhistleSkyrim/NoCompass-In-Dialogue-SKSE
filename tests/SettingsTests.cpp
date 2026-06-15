#include "TestHarness.h"

#include "Settings.h"

namespace DialogueHUDFade::Tests
{
    void InvalidIniValuesAreClamped()
    {
        const auto parsed = ParseSettings(R"ini(
[Animation]
fFadeOutSeconds=-1
fFadeInSeconds=25
fHiddenAlpha=-10
fVisibleAlpha=250
sEasing=not-real
)ini");

        DHF_REQUIRE_NEAR(parsed.settings.fadeOutSeconds, 0.0F, 0.01F);
        DHF_REQUIRE_NEAR(parsed.settings.fadeInSeconds, 10.0F, 0.01F);
        DHF_REQUIRE_NEAR(parsed.settings.hiddenAlpha, 0.0F, 0.01F);
        DHF_REQUIRE_NEAR(parsed.settings.visibleAlpha, 100.0F, 0.01F);
        DHF_REQUIRE(parsed.settings.easing == Easing::kSmoothstep);
        DHF_REQUIRE(parsed.warnings.size() >= 5);
    }

    void IniPathsAreParsed()
    {
        const auto parsed = ParseSettings(R"ini(
[Targets]
sFullHUDPaths=_root.HUDMovieBaseInstance,_root.Custom
)ini");

        DHF_REQUIRE(parsed.settings.fullHudPaths.size() == 2);
        DHF_REQUIRE(parsed.settings.fullHudPaths[0] == "_root.HUDMovieBaseInstance");
        DHF_REQUIRE(parsed.settings.fullHudPaths[1] == "_root.Custom");
    }

    void NonFiniteIniNumbersUseDefaults()
    {
        const auto parsed = ParseSettings(R"ini(
[Animation]
fFadeOutSeconds=nan
fFadeInSeconds=inf
fHiddenAlpha=-inf
)ini");

        DHF_REQUIRE_NEAR(parsed.settings.fadeOutSeconds, 0.20F, 0.01F);
        DHF_REQUIRE_NEAR(parsed.settings.fadeInSeconds, 0.20F, 0.01F);
        DHF_REQUIRE_NEAR(parsed.settings.hiddenAlpha, 0.0F, 0.01F);
        DHF_REQUIRE(parsed.warnings.size() >= 3);
    }

    void IniCommentsAndQuotedValuesAreParsed()
    {
        const auto parsed = ParseSettings(R"ini(
[Animation]
fFadeOutSeconds=0.5 # seconds
sEasing="ease-out-cubic"

[Targets]
sFullHUDPaths="_root.HUD#Special", '_root.Custom' ; trailing comment
)ini");

        DHF_REQUIRE_NEAR(parsed.settings.fadeOutSeconds, 0.5F, 0.01F);
        DHF_REQUIRE(parsed.settings.easing == Easing::kEaseOutCubic);
        DHF_REQUIRE(parsed.settings.fullHudPaths.size() == 2);
        DHF_REQUIRE(parsed.settings.fullHudPaths[0] == "_root.HUD#Special");
        DHF_REQUIRE(parsed.settings.fullHudPaths[1] == "_root.Custom");
    }

    void QuotedScalarValuesAreParsed()
    {
        const auto parsed = ParseSettings(R"ini(
[General]
bEnabled="off"

[Animation]
fFadeOutSeconds='0.75'
sEasing="linear"

[Targets]
sTargetMode='FullHUD'
)ini");

        DHF_REQUIRE(!parsed.settings.enabled);
        DHF_REQUIRE_NEAR(parsed.settings.fadeOutSeconds, 0.75F, 0.01F);
        DHF_REQUIRE(parsed.settings.easing == Easing::kLinear);
        DHF_REQUIRE(parsed.warnings.empty());
    }

    void IniPathListKeepsQuotedCommas()
    {
        const auto parsed = ParseSettings(R"ini(
[Targets]
sFullHUDPaths="_root.HUD,Special",_root.Custom
)ini");

        DHF_REQUIRE(parsed.settings.fullHudPaths.size() == 2);
        DHF_REQUIRE(parsed.settings.fullHudPaths[0] == "_root.HUD,Special");
        DHF_REQUIRE(parsed.settings.fullHudPaths[1] == "_root.Custom");
    }

    void HiddenAlphaCannotExceedVisibleAlpha()
    {
        const auto parsed = ParseSettings(R"ini(
[Animation]
fHiddenAlpha=80
fVisibleAlpha=20
)ini");

        DHF_REQUIRE_NEAR(parsed.settings.hiddenAlpha, 20.0F, 0.01F);
        DHF_REQUIRE_NEAR(parsed.settings.visibleAlpha, 80.0F, 0.01F);
        DHF_REQUIRE(!parsed.warnings.empty());
    }
}

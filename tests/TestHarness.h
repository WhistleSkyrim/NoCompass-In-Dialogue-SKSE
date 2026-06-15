#pragma once

#include "HudFader.h"

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace DialogueHUDFade::Tests
{
    struct TestFailure : std::runtime_error
    {
        using std::runtime_error::runtime_error;
    };

    inline void Require(bool a_condition, const char* a_expression, const char* a_file, int a_line)
    {
        if (!a_condition) {
            throw TestFailure(std::string(a_file) + ":" + std::to_string(a_line) + " failed: " + a_expression);
        }
    }

    inline void RequireNear(float a_lhs, float a_rhs, float a_tolerance, const char* a_expression, const char* a_file, int a_line)
    {
        if (std::fabs(a_lhs - a_rhs) > a_tolerance) {
            throw TestFailure(
                std::string(a_file) + ":" + std::to_string(a_line) + " failed: " + a_expression +
                " lhs=" + std::to_string(a_lhs) + " rhs=" + std::to_string(a_rhs));
        }
    }

    class MockHudTarget final : public IHudTarget
    {
    public:
        HudState Read() override
        {
            return HudState{ .alpha = alpha, .visible = visible, .available = available };
        }

        bool ApplyAlpha(float a_alpha) override
        {
            if (!available || rejectApply) {
                return false;
            }

            alpha = a_alpha;
            appliedValues.push_back(a_alpha);
            return true;
        }

        std::optional<float> alpha{ 100.0F };
        std::optional<bool> visible{ true };
        bool available{ true };
        bool rejectApply{ false };
        std::vector<float> appliedValues{};
    };
}

#define DHF_REQUIRE(expr) ::DialogueHUDFade::Tests::Require((expr), #expr, __FILE__, __LINE__)
#define DHF_REQUIRE_NEAR(lhs, rhs, tolerance) ::DialogueHUDFade::Tests::RequireNear((lhs), (rhs), (tolerance), #lhs " ~= " #rhs, __FILE__, __LINE__)

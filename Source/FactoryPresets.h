#pragma once

#include "GlitchEngine.h"

namespace beatflip
{
struct FactoryPreset
{
    const char* name;
    Settings settings;
};

inline const std::array<FactoryPreset, 6>& factoryPresets() noexcept
{
    static const auto presets = []
    {
        std::array<FactoryPreset, 6> result {};
        const auto add = [&result] (int index, const char* name, std::uint32_t seed,
                                  float amount, float mix, std::uint8_t mask, int repeats,
                                  float swing, int autoBars, bool protect)
        {
            auto& preset = result[static_cast<std::size_t> (index)];
            preset.name = name;
            preset.settings.seed = seed;
            preset.settings.enabled = true;
            preset.settings.amount = amount;
            preset.settings.mix = mix;
            preset.settings.pattern = { mask, repeats, protect };
            preset.settings.swing = swing;
            preset.settings.autoFlipBars = autoBars;
        };
        add (0, "Subtle Pocket", 219, 0.35f, 0.45f,
             effectBit (Effect::repeat) | effectBit (Effect::shuffle), 2, 0.20f, 0, true);
        add (1, "Stutter Lab", 8123, 0.90f, 1.0f, effectBit (Effect::repeat), 8, 0.0f, 0, true);
        add (2, "Reverse Cuts", 533, 0.80f, 0.85f,
             effectBit (Effect::reverse) | effectBit (Effect::gate), 4, 0.15f, 0, true);
        add (3, "Half-time", 1117, 0.85f, 1.0f, effectBit (Effect::halfSpeed), 0, 0.0f, 0, true);
        add (4, "Evolving Groove", 9031, 0.65f, 0.80f, allEffects, 4, 0.30f, 2, true);
        add (5, "Full Chaos", 27183, 1.0f, 1.0f, allEffects, 16, 0.0f, 1, false);
        return result;
    }();
    return presets;
}
}

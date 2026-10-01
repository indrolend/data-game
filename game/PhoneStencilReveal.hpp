#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string>

namespace phone_stencil {

constexpr float HorizontalScale = 0.84f;
constexpr float TrackingEm = 0.018f;

inline std::uint32_t textSeed(const std::string& text) {
    std::uint32_t seed = 2166136261u;
    for (unsigned char c : text) seed = (seed ^ c) * 16777619u;
    return seed;
}

inline std::uint32_t glyphSeed(std::uint32_t seed, unsigned char c, std::size_t index) {
    return (seed ^ (static_cast<std::uint32_t>(c) + static_cast<std::uint32_t>(index) * 131u)) * 16777619u;
}

inline float appearanceAge(float transitionProgress) {
    return std::clamp(transitionProgress, 0.0f, 1.0f) * 0.24f;
}

inline float glyphReveal(float age, std::size_t index, std::size_t count, bool leftToRight) {
    const float order = count <= 1 ? 0.0f : static_cast<float>(index) / static_cast<float>(count - 1);
    const float directionalOrder = leftToRight ? order : 1.0f - order;
    const float reveal = std::clamp((std::max(0.0f, age) - directionalOrder * 0.105f) / 0.085f, 0.0f, 1.0f);
    return reveal * reveal * (3.0f - 2.0f * reveal);
}

} // namespace phone_stencil

#pragma once
#include <algorithm>
#include <cmath>
#include <vector>

// Shared by the processor and editor. No JUCE dependencies, so curve
// interpolation and input normalization can be tested with a plain C++ compiler.
struct CurveNode { float x = 0.f, y = 0.f, tension = 0.f; };

namespace CurveMath {
inline constexpr float minSpacing = 0.0001f;
inline float clamp(float value, float low, float high) noexcept {
    return std::min(high, std::max(low, value));
}
inline float segmentValue(const CurveNode& a, const CurveNode& b, float x) noexcept {
    const float t = clamp((x - a.x) / std::max(minSpacing, b.x - a.x), 0.f, 1.f);
    return clamp(a.y + (b.y - a.y) * t + a.tension * 4.f * t * (1.f - t), -2.f, 2.f);
}
inline float evaluate(const std::vector<CurveNode>& nodes, float x) noexcept {
    if (nodes.empty() || !std::isfinite(x)) return 0.f;
    if (x <= nodes.front().x) return nodes.front().y;
    if (x >= nodes.back().x) return nodes.back().y;
    const auto next = std::upper_bound(nodes.begin(), nodes.end(), x,
        [](float pos, const CurveNode& node) { return pos < node.x; });
    return segmentValue(*(next - 1), *next, x);
}
inline void normalize(std::vector<CurveNode>& nodes) {
    if (nodes.size() > 512) nodes.resize(512);
    for (auto& n : nodes) {
        if (!std::isfinite(n.x)) n.x = 0.f;
        if (!std::isfinite(n.y)) n.y = 0.f;
        if (!std::isfinite(n.tension)) n.tension = 0.f;
        n.x = clamp(n.x, 0.f, 1.f);
        n.y = clamp(n.y, -2.f, 2.f);
        n.tension = clamp(n.tension, -2.f, 2.f);
    }
    std::stable_sort(nodes.begin(), nodes.end(),
        [](const CurveNode& a, const CurveNode& b) { return a.x < b.x; });
    std::vector<CurveNode> merged;
    merged.reserve(nodes.size() + 2);
    for (const auto& n : nodes) {
        if (!merged.empty() && n.x - merged.back().x < minSpacing)
            merged.back() = n;
        else
            merged.push_back(n);
    }
    if (merged.empty()) merged = {{0.f, 0.f, 0.f}, {1.f, 0.f, 0.f}};
    if (merged.size() == 1) {
        if (merged.front().x > 0.f) merged.insert(merged.begin(), {0.f, merged.front().y, 0.f});
        else merged.push_back({1.f, merged.front().y, 0.f});
    }
    // Keep the start and end anchored to the full envelope cycle.
    // Existing envelopes without endpoints get constant-value boundary extensions.
    if (merged.front().x > 0.f && merged.size() < 512)
        merged.insert(merged.begin(), {0.f, merged.front().y, 0.f});
    else merged.front().x = 0.f;
    if (merged.back().x < 1.f && merged.size() < 512)
        merged.push_back({1.f, merged.back().y, 0.f});
    else merged.back().x = 1.f;
    nodes.swap(merged);
}
}

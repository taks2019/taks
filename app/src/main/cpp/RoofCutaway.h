#pragma once
#include <algorithm>
#include <cmath>

// TAKS World: rendering-only roof reveal for orthographic isometric camera.
// Zoom is the visible world width: smaller = closer. No simulation or save state changes.
namespace taks::RoofCutaway {
constexpr float hiddenAtZoom = 8.0f;
constexpr float fullyVisibleAtZoom = 14.0f;
inline float visibility(float zoom) {
    // Invalid camera input must never accidentally open up the cave.
    if (!std::isfinite(zoom)) return 1.0f;
    return std::clamp((zoom - hiddenAtZoom) /
                      (fullyVisibleAtZoom - hiddenAtZoom), 0.0f, 1.0f);
}
}

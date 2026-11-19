#pragma once

#include <algorithm>
#include <cmath>

namespace taks {

// Regional gameplay profile for the Ghaap/Kuruman summer-rainfall biome.
// Values are tunable gameplay estimates, not a paleoclimate reconstruction.
// Every value derives from the saved clock, so the save format stays stable.
struct AfricanWeather {
    double wetSeason = 0.0;   // 0 dry winter .. 1 summer-rain season
    double humidity = 0.35;
    double precipitation = 0.0;
    double temperatureC = 20.0;
    double windStrength = 0.25; // normalized gameplay force, 0..1
    double windX = 0.7;
    double windZ = 0.7;
};

inline AfricanWeather africanWeatherAt(double hours) {
    constexpr double tau = 6.2831853071795864769;
    auto clamp = [](double value, double low, double high) {
        return std::max(low, std::min(high, value));
    };
    auto smooth = [](double value) {
        value = std::max(0.0, std::min(1.0, value));
        return value * value * (3.0 - 2.0 * value);
    };

    if (!std::isfinite(hours) || hours < 0.0) hours = 0.0;
    const double day = std::fmod(hours / 24.0, 365.0);
    const double hour = std::fmod(hours, 24.0);
    const double annualPhase = tau * (day - 15.0) / 365.0; // warm/wet peak in January
    const double annualCos = std::cos(annualPhase);

    AfricanWeather weather;
    // A broad summer wet window with smooth shoulders and a dry winter.
    weather.wetSeason = smooth((annualCos + 0.50) / 1.50);

    // Deterministic atmospheric cycles make showers last several game hours,
    // with calmer spells and occasional stronger convective storms.
    const double slowFront = std::sin(hours * 0.013 + 2.1);
    const double front = std::sin(hours * 0.047 + 1.4 + slowFront * 0.35);
    const double convection = std::sin(hours * 0.17 + std::cos(hours * 0.031));
    const double stormSignal = 0.48 * convection + 0.34 * front + 0.18 * slowFront;
    const double stormPulse = smooth((stormSignal + 0.22) / 1.22);
    weather.precipitation = weather.wetSeason * stormPulse;
    weather.humidity = clamp(0.28 + 0.40 * weather.wetSeason + 0.24 * stormPulse,
                             0.12, 0.98);

    const double dayPhase = tau * (hour - 14.0) / 24.0;
    const double winter = 1.0 - weather.wetSeason;
    const double clearNight = std::pow(std::max(0.0, std::cos(tau * (hour - 2.0) / 24.0)), 2.0);
    weather.temperatureC = 19.5 + 6.5 * annualCos + 5.1 * std::cos(dayPhase)
                         - 9.0 * winter * clearNight * (1.0 - 0.65 * weather.humidity);

    const double breeze = 0.5 + 0.5 * std::sin(hours * 0.092 + slowFront);
    const double dryGust = winter * std::pow(std::sin(hours * 0.029 + 0.7), 2.0);
    weather.windStrength = clamp(0.12 + 0.24 * breeze + 0.31 * stormPulse
                                 + 0.20 * dryGust, 0.08, 0.90);
    const double direction = 0.55 + 0.42 * std::sin(hours * 0.021 + 0.4)
                           + 0.16 * std::sin(hours * 0.083 + slowFront);
    weather.windX = std::cos(direction);
    weather.windZ = std::sin(direction);
    return weather;
}

} // namespace taks

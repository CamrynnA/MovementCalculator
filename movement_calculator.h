#pragma once

#include <array>
#include <string>

namespace movement {
inline constexpr int kTerrainCount = 6;
inline constexpr int kMaxTilesPerTerrain = 999;
inline constexpr double kStaminaPerTurn = 12.0;
inline constexpr double kMaxRouteCost = 10000.0;

struct RouteResult {
    double totalCost{};
    int turns{};
    double finalTurnCost{};
    double staminaRemaining{};
    bool overLimit{};
    bool discountedRates{};
};

// Uses standard costs by default; pass true for Paramount / Crown House costs.
RouteResult calculate(const std::array<int, kTerrainCount>& tileCounts, bool useParamountOrCrownRates = false);
// Formats the total cost and turn-by-turn route result.
std::wstring formatSummary(const RouteResult& route);
}

#include "movement_calculator.h"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>

namespace movement {
RouteResult calculate(const std::array<int, kTerrainCount>& n, bool useParamountOrCrownRates) {
    RouteResult route;
    route.discountedRates = useParamountOrCrownRates;
    // The selected rates change regular terrain, allied roads, and forests.
    const double regularRate = useParamountOrCrownRates ? 0.75 : 1.0;
    const double alliedRoadRate = useParamountOrCrownRates ? 0.5 : 0.75;
    const double forestRate = useParamountOrCrownRates ? 1.5 : 2.0;
    route.totalCost =
        n[0] * regularRate +
        n[1] * alliedRoadRate +
        n[2] * 1.0 +   // non-allied road rate remains 1.0
        n[3] * forestRate +
        n[4] * 0.5 +
        n[5] * 1.5;
    // Skip the turn breakdown if the route is over the limit.
    route.overLimit = route.totalCost > kMaxRouteCost;
    if (route.overLimit) return route;

    route.turns = std::max(1, static_cast<int>(std::ceil(route.totalCost / kStaminaPerTurn)));
    route.finalTurnCost = route.totalCost - (route.turns - 1) * kStaminaPerTurn;
    route.staminaRemaining = kStaminaPerTurn - route.finalTurnCost;
    return route;
}

std::wstring formatSummary(const RouteResult& route) {
    if (route.overLimit) {
        return L"Route exceeds the 10,000 stamina limit. Reduce tile counts and calculate again.";
    }
    std::wostringstream out;
    out << std::fixed << std::setprecision(2);
    out << (route.discountedRates ? L"PARAMOUNT / CROWN HOUSE COSTS" : L"STANDARD COSTS") << L"\r\n\r\n";
    out << L"TOTAL ROUTE COST\r\n" << route.totalCost << L" stamina\r\n\r\n";
    if (route.turns == 1) {
        out << L"ONE TURN\r\nStamina remaining: " << route.staminaRemaining << L"\r\n";
    } else {
        out << L"ROUTE BREAKDOWN\r\n";
        for (int turn = 1; turn < route.turns; ++turn) {
            out << L"Turn " << turn << L": 12.00 stamina — exhausted\r\n";
        }
        out << L"Turn " << route.turns << L": " << route.finalTurnCost
            << L" stamina\r\n\r\nStamina remaining at destination: "
            << route.staminaRemaining << L"\r\n";
    }
    return out.str();
}
}

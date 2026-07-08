#pragma once
#include "GameState.h"
#include <string>

// Owns the day-by-day season loop: advancing the calendar one day at a time,
// rolling DayPhase/CurrentWeek forward, and telling CareerHubPage what to
// display and when a matchday or the season itself has arrived/ended.
namespace CareerDayService
{
    // Sets up SeasonStartDate/SeasonEndDate/CurrentDate/CurrentDay for a brand
    // new career. Call this once at career creation (same point where
    // TeamAssignmentPage resets CurrentWeek/LastChoice), not in a constructor.
    void InitializeSeason(int startYear);

    // Advances exactly one calendar day. Rolls CurrentWeek forward when a new
    // round begins (Monday). Returns true if the new day is Saturday (matchday).
    bool AdvanceDay();

    // Display helpers for CareerHubPage
    std::wstring GetDayPhaseName(DayPhase day);
    std::wstring GetTodayLabel(); // e.g. "Wednesday, 15 April 2026"

    bool IsMatchday();
    bool IsSeasonComplete();
}
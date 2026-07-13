#pragma once
#include <string>
#include <vector>
#include <unordered_map>

// Random day-events: small interrupts that can fire on a Monday-Friday
// day-advance, presented as a 2-3 choice popup, applying an immediate
// stat delta on top of whatever the block allocation does for the week.
// Content lives in Assets\Data\events.csv so new events don't need a
// recompile - same pattern as localteams.csv/stateteams.csv/aflteams.csv.
namespace DayEventService
{
    struct EventChoice
    {
        std::wstring Label;
        // Stat name (Fatigue, InjuryRisk, RecoveryQuality, Confidence,
        // Stress, Motivation, Discipline, Finances, Relationships) -> delta.
        std::unordered_map<std::wstring, int> StatDeltas;
    };

    struct DayEvent
    {
        std::wstring EventId;
        std::wstring Title;
        std::wstring Description;
        std::vector<EventChoice> Choices; // always 2 or 3
    };

    // Loads events.csv, searching the same candidate paths as the rest of
    // the project's CSV loading. Returns an empty vector if the file can't
    // be found - callers should treat that as "no events this session"
    // rather than a hard failure.
    std::vector<DayEvent> LoadEvents();

    // Rolls a percentChance (0-100) chance of an event firing. Returns
    // nullptr if it doesn't fire or if events is empty.
    DayEvent const* RollForEvent(std::vector<DayEvent> const& events, int percentChance);
}
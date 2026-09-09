#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>

// Loads CSV-driven day events (Assets\Data\events.csv) and rolls whether
// one triggers on a given Mon-Fri day advance.
namespace DayEventService
{
	struct EventChoice
	{
		std::wstring Label;
		std::unordered_map<std::wstring, int> StatDeltas;

		// If non-empty, picking this choice sets this flag in
		// GameState::StoryFlags - the mechanism the branching narrative
		// system (multi-stage arcs) is built on. Empty means this choice
		// doesn't set anything.
		std::wstring SetFlag;
	};

	struct DayEvent
	{
		std::wstring EventId;
		std::wstring Title;
		std::wstring Description;
		std::vector<EventChoice> Choices;

		std::wstring RequiresFlag;
		std::wstring ExcludesFlag;
	};


	std::vector<DayEvent> LoadEvents();


	DayEvent const* RollForEvent(
		std::vector<DayEvent> const& events,
		int percentChance,
		std::unordered_set<std::wstring> const& activeFlags);
}
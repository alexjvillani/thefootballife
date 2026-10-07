#pragma once
#include "NarrativeState.h"

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>

// Loads CSV-driven day events (Assets\Data\events.csv) and rolls whether
// one triggers on a given Mon-Fri day advance.
namespace DayEventService
{
	// One term of a RequiresCounters condition, e.g. "FamilyBond>=2".
	struct CounterCondition
	{
		std::wstring Name;
		std::wstring Op;   // one of: >=  <=  >  <  ==  !=
		int Value{ 0 };
	};

	struct EventChoice
	{
		std::wstring Label;
		std::unordered_map<std::wstring, int> StatDeltas;

		// Branching-narrative effects of picking this choice (applied by
		// ApplyNarrativeEffects). ClearFlags are removed first, then
		// SetFlags are added, then CounterDeltas are applied.
		std::vector<std::wstring> SetFlags;
		std::vector<std::wstring> ClearFlags;
		std::unordered_map<std::wstring, int> CounterDeltas;
	};

	struct DayEvent
	{
		std::wstring EventId;
		std::wstring Title;
		std::wstring Description;
		std::vector<EventChoice> Choices;

		// Eligibility (all must hold for the event to be able to roll):
		std::vector<std::wstring> RequiresFlags;     // every one of these flags must be set
		std::vector<std::wstring> RequiresAnyFlags;  // if non-empty, at least one must be set
		std::vector<std::wstring> ExcludesFlags;     // none of these may be set
		std::vector<CounterCondition> RequiresCounters; // every condition must hold

		// false = one-shot: once the player has made a choice in this
		// event (it's in NarrativeState::Log) it never rolls again.
		// Blank/absent in the CSV means true, so ambient events behave as
		// they always did.
		bool Repeatable{ true };

		// Relative odds of being picked once the daily roll has succeeded and
		// the event is eligible. Blank/absent in the CSV means 1. Give arc
		// entry and follow-up events a higher weight so story beats arrive
		// reasonably soon instead of being buried among ambient events.
		int Weight{ 1 };
	};


	std::vector<DayEvent> LoadEvents();

	bool IsEligible(DayEvent const& event, NarrativeState const& narrative);

	DayEvent const* RollForEvent(
		std::vector<DayEvent> const& events,
		int percentChance,
		NarrativeState const& narrative);

	// Clears/sets flags and applies counter deltas for a picked choice.
	// Does NOT write the log entry - the caller does that with
	// NarrativeState::Record, since it knows the event id, season and week.
	void ApplyNarrativeEffects(EventChoice const& choice, NarrativeState& narrative);
}
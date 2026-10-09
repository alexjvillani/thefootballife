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
	// Live personal stats by name (Confidence, Relationships, Academics...),
	// supplied by the caller so events can be gated on how the player is doing.
	using StatMap = std::unordered_map<std::wstring, int>;

	// One term of a RequiresCounters / RequiresStats condition, e.g. "FamilyBond>=2".
	struct CounterCondition
	{
		std::wstring Name;
		std::wstring Op;   // one of: >=  <=  >  <  ==  !=
		int Value{ 0 };
	};

	// One term of a RequiresFact / ExcludesFact condition: "partner" (the
	// fact is set) or "partner=Mia" (the fact has exactly that value).
	struct FactCondition
	{
		std::wstring Key;
		std::wstring Value;
		bool HasValue{ false };
	};

	enum class RepeatRule
	{
		Always,     // may fire any number of times (the default)
		Never,      // one-shot: never again once decided
		PerSeason   // at most once per career season
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
		std::unordered_map<std::wstring, int> CounterSets; // "Name=0": set to an exact value (applied before the deltas)

		// Facts to store (key, raw value) and facts to remove. A raw value
		// may list options separated by '|' (one is picked at random) or
		// refer to another fact as {name}. Values are resolved against the
		// state from BEFORE this choice, so "ex_partner={partner}" plus
		// clearing "partner" in the same choice works.
		std::vector<std::pair<std::wstring, std::wstring>> SetFacts;
		std::vector<std::wstring> ClearFacts;
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
		std::vector<CounterCondition> RequiresStats;    // personal stats, e.g. "Confidence>=60;Academics>=50"

		std::vector<FactCondition> RequiresFacts;    // every one must hold
		std::vector<FactCondition> ExcludesFacts;    // none may hold

		// Blank/absent in the CSV means Always, so ambient events behave as
		// they always did. 0/false/no/never = Never, "season" = PerSeason.
		RepeatRule Repeat{ RepeatRule::Always };

		// Relative odds of being picked once the daily roll has succeeded and
		// the event is eligible. Blank/absent in the CSV means 1. Give arc
		// entry and follow-up events a higher weight so story beats arrive
		// reasonably soon instead of being buried among ambient events.
		int Weight{ 1 };
	};


	std::vector<DayEvent> LoadEvents();

	// currentSeason is the career season number (1 = first), used by
	// RepeatRule::PerSeason.
	bool IsEligible(DayEvent const& event, NarrativeState const& narrative, int currentSeason = 0, StatMap const& stats = {});

	DayEvent const* RollForEvent(
		std::vector<DayEvent> const& events,
		int percentChance,
		NarrativeState const& narrative,
		int currentSeason = 0,
		StatMap const& stats = {});

	// Clears/sets flags and facts and applies counter deltas for a picked choice.
	// Does NOT write the log entry - the caller does that with
	// NarrativeState::Record, since it knows the event id, season and week.
	// `stats` (live personal stats) is only needed for "@jobs:" fact values.
	void ApplyNarrativeEffects(EventChoice const& choice, NarrativeState& narrative, StatMap const& stats = {});

	// ---- Data tables (Assets\Data\names.csv, jobs.csv, epilogue.csv) ----
	// All three are optional: a missing file just means no names/jobs/epilogue.

	// A random name from names.csv for a category ("partner", "sponsor"...),
	// or "" if the category is missing or empty. In an event's SetFact column,
	// write  partner=@names:partner  to use it.
	std::wstring PickName(std::wstring const& category);

	struct JobDef
	{
		std::wstring Id;
		std::wstring Title;
		std::wstring Employer;
		std::wstring Tier;
		int MinAcademics{ 0 };
		int PayPerBlock{ 6 };   // Finances gained per Work block while holding this job
		std::wstring Description;
	};

	// The job whose Title matches (the "job" fact), or nullptr.
	JobDef const* FindJobByTitle(std::wstring const& title);

	// Picks a job the player qualifies for (MinAcademics <= their Academics).
	// tier = "entry", "mid", "good"... for that tier, or "best" for a random job
	// from the highest tier they qualify for - better Academics, better jobs.
	// nullptr if they don't qualify for anything.
	JobDef const* PickJob(std::wstring const& tier, StatMap const& stats);

	// Builds the retirement epilogue from epilogue.csv for what the story
	// remembers. Returns "" if the file is missing, so the caller can fall back.
	std::wstring BuildEpilogue(NarrativeState const& narrative, StatMap const& stats);
}
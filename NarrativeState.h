#pragma once
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

// One decision the player made in a day event, recorded when they pick a
// choice. Season is the career season number (1 = first season), Week is
// the in-season week, Choice is the 0-based index of the option picked.
struct NarrativeLogEntry
{
	std::wstring EventId;
	int Season{ 1 };
	int Week{ 1 };
	int Choice{ 0 };
};

// Everything the game remembers about the player's story so far.
// Career-scoped: cleared on true new-career creation (TeamAssignmentPage),
// deliberately NOT cleared by season rollover or Draft Night promotion - a
// choice made in Local tier should still matter after being drafted.
//
//  - Flags:    on/off facts ("mentor_accepted"). Names must not contain
//              ',' or ';' (',' is the save-file separator, ';' is the
//              events.csv list separator).
//  - Counters: running tallies of the kind of player you're becoming
//              ("FamilyBond", "Ambition"). Clamped to [-100, 100].
//  - Facts:    named values the story needs to talk about ("partner" =
//              "Mia", "sponsor" = "Stride Footwear"). Event text can use
//              them as {partner}; events can require or exclude them.
//  - Log:      every event decision, in order. This is what lets one-shot
//              events stay one-shot and lets later events/UI look back.
struct NarrativeState
{
	std::unordered_set<std::wstring> Flags;
	std::unordered_map<std::wstring, int> Counters;
	std::unordered_map<std::wstring, std::wstring> Facts;
	std::vector<NarrativeLogEntry> Log;

	bool HasFlag(std::wstring const& flag) const
	{
		return Flags.count(flag) > 0;
	}

	int Counter(std::wstring const& name) const
	{
		auto it = Counters.find(name);
		return it != Counters.end() ? it->second : 0;
	}

	bool HasFact(std::wstring const& key) const
	{
		auto it = Facts.find(key);
		return it != Facts.end() && !it->second.empty();
	}

	std::wstring Fact(std::wstring const& key) const
	{
		auto it = Facts.find(key);
		return it != Facts.end() ? it->second : std::wstring();
	}

	// Replaces {name} with that fact's value, or {name|fallback} with the
	// fallback when the fact isn't set. A missing fact with no fallback
	// becomes "someone", so a text never shows a raw placeholder.
	std::wstring Format(std::wstring const& text) const
	{
		std::wstring out;
		out.reserve(text.size());
		for (size_t i = 0; i < text.size(); ++i)
		{
			if (text[i] != L'{') { out += text[i]; continue; }
			size_t close = text.find(L'}', i + 1);
			if (close == std::wstring::npos) { out += text.substr(i); break; }

			std::wstring token = text.substr(i + 1, close - i - 1);
			std::wstring key = token;
			std::wstring fallback = L"someone";
			size_t bar = token.find(L'|');
			if (bar != std::wstring::npos)
			{
				key = token.substr(0, bar);
				fallback = token.substr(bar + 1);
			}
			out += HasFact(key) ? Fact(key) : fallback;
			i = close;
		}
		return out;
	}

	bool HasSeen(std::wstring const& eventId) const
	{
		for (auto const& entry : Log)
		{
			if (entry.EventId == eventId) return true;
		}
		return false;
	}

	bool HasSeenInSeason(std::wstring const& eventId, int season) const
	{
		for (auto const& entry : Log)
		{
			if (entry.EventId == eventId && entry.Season == season) return true;
		}
		return false;
	}

	// 0-based choice index the player picked the most recent time this
	// event fired, or -1 if it has never fired.
	int LastChoiceFor(std::wstring const& eventId) const
	{
		for (auto it = Log.rbegin(); it != Log.rend(); ++it)
		{
			if (it->EventId == eventId) return it->Choice;
		}
		return -1;
	}

	void Record(std::wstring const& eventId, int season, int week, int choice)
	{
		NarrativeLogEntry entry;
		entry.EventId = eventId;
		entry.Season = season;
		entry.Week = week;
		entry.Choice = choice;
		Log.push_back(std::move(entry));
	}

	void Clear()
	{
		Flags.clear();
		Counters.clear();
		Facts.clear();
		Log.clear();
	}
};
#pragma once
#include "PlayerData.h"
#include "FixtureService.h"
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace SaveGameService
{
	constexpr int MaxSaveSlots = 3;

	// Rolling autosave history: index 1 is the newest, MaxAutosaves the oldest.
	// Kept entirely separate from the manual slots above so an autosave can
	// never overwrite a save the player made on purpose.
	//
	// Index 0 is a separate "latest" recovery save: overwritten in place (no
	// history) every time something changes, so a crash or a killed process
	// (e.g. Visual Studio's Stop Debugging) loses at most the last action.
	constexpr int MaxAutosaves = 3;

	struct TeamSeasonStats
	{
		int wins{ 0 };
		int losses{ 0 };
		int draws{ 0 };
		int pointsFor{ 0 };
		int pointsAgainst{ 0 };
	};

	struct PersonalStats
	{
		int fatigue{ 30 };
		int injuryRisk{ 20 };
		int recoveryQuality{ 55 };
		int confidence{ 55 };
		int stress{ 35 };
		int motivation{ 60 };
		int discipline{ 60 };
		int finances{ 35 };
		int relationships{ 50 };

		int trainingBlocks{ 4 };
		int schoolBlocks{ 5 };
		int workBlocks{ 2 };
		int socialBlocks{ 2 };
		int recoveryBlocks{ 1 };

		// Grouped in here for save-format convenience rather than because
		// they're "personal life stats" like the rest of this struct.
		// gamesPlayed is a career total - never reset. seasonVotes is the
		// player's running Best & Fairest vote tally - reset to 0 at the
		// start of each new season.
		int gamesPlayed{ 0 };
		int seasonVotes{ 0 };

		// Career totals for the eventual retirement/legacy screen - never
		// reset by season rollover or Draft Night, same as gamesPlayed.
		int careerSeasonsPlayed{ 0 };
		int careerBestAndFairestWins{ 0 };
	};

	// Mirrors GameState's SimpleDate/DayPhase as plain ints so this header
	// doesn't need to include GameState.h (which already includes this header).
	// currentDayPhase uses the same ordinal values as the DayPhase enum
	// (Monday=0 ... Sunday=6).
	struct CalendarState
	{
		int currentYear{ 2026 };
		int currentMonth{ 3 };
		int currentDay{ 27 };
		int currentDayPhase{ 0 };
		int seasonStartYear{ 2026 };
		int seasonStartMonth{ 3 };
		int seasonStartDay{ 27 };
		int seasonEndYear{ 2026 };
		int seasonEndMonth{ 8 };
		int seasonEndDay{ 30 };
	};

	std::wstring GetSaveFolder();
	std::wstring GetSaveSlotPath(int slot);
	bool SlotExists(int slot);
	int  FindFirstAvailableSlot();

	bool SaveToSlot(
		int slot,
		PlayerData const& player,
		int currentWeek,
		std::wstring const& lastChoice,
		std::unordered_map<std::wstring, TeamSeasonStats> const& teamStats = {},
		std::vector<FixtureService::Fixture> const& fixtures = {},
		CalendarState const& calendar = {},
		PersonalStats const& personalStats = {},
		std::unordered_set<std::wstring> const& storyFlags = {}
	);

	bool LoadFromSlot(
		int slot,
		PlayerData& player,
		int& currentWeek,
		std::wstring& lastChoice,
		std::unordered_map<std::wstring, TeamSeasonStats>& teamStats,
		std::vector<FixtureService::Fixture>& fixtures,
		CalendarState& calendar,
		PersonalStats& personalStats,
		std::unordered_set<std::wstring>& storyFlags
	);

	bool GetSavePreview(int slot, std::wstring& playerName, int& week);
	bool DeleteSlot(int slot);

	// ---- Path-based core (shared by manual slots and autosaves) ----
	// Writes via a temp file + atomic rename, so a crash mid-save can't
	// leave a half-written file behind.
	bool SaveToPath(
		std::wstring const& path,
		PlayerData const& player,
		int currentWeek,
		std::wstring const& lastChoice,
		std::unordered_map<std::wstring, TeamSeasonStats> const& teamStats,
		std::vector<FixtureService::Fixture> const& fixtures,
		CalendarState const& calendar,
		PersonalStats const& personalStats,
		std::unordered_set<std::wstring> const& storyFlags
	);

	bool LoadFromPath(
		std::wstring const& path,
		PlayerData& player,
		int& currentWeek,
		std::wstring& lastChoice,
		std::unordered_map<std::wstring, TeamSeasonStats>& teamStats,
		std::vector<FixtureService::Fixture>& fixtures,
		CalendarState& calendar,
		PersonalStats& personalStats,
		std::unordered_set<std::wstring>& storyFlags
	);

	bool GetSavePreviewFromPath(std::wstring const& path, std::wstring& playerName, int& week);

	// ---- Autosave ----
	std::wstring GetAutosavePath(int index);   // 0 = latest recovery save; 1 = newest .. MaxAutosaves = oldest
	bool AutosaveExists(int index);

	// Writes a new autosave and rolls the older ones down (1 -> 2 -> 3, the
	// oldest is discarded). The existing history is only touched once the
	// new save has been written successfully.
	bool Autosave(
		PlayerData const& player,
		int currentWeek,
		std::wstring const& lastChoice,
		std::unordered_map<std::wstring, TeamSeasonStats> const& teamStats,
		std::vector<FixtureService::Fixture> const& fixtures,
		CalendarState const& calendar,
		PersonalStats const& personalStats,
		std::unordered_set<std::wstring> const& storyFlags
	);

	// Overwrites the "latest" recovery save (index 0) in place. Cheap and
	// atomic; does not touch the rolling history.
	bool AutosaveLatest(
		PlayerData const& player,
		int currentWeek,
		std::wstring const& lastChoice,
		std::unordered_map<std::wstring, TeamSeasonStats> const& teamStats,
		std::vector<FixtureService::Fixture> const& fixtures,
		CalendarState const& calendar,
		PersonalStats const& personalStats,
		std::unordered_set<std::wstring> const& storyFlags
	);

	// Same shape as LoadFromSlot, so the load screen can treat an autosave
	// exactly like a normal slot.
	bool LoadAutosave(
		int index,
		PlayerData& player,
		int& currentWeek,
		std::wstring& lastChoice,
		std::unordered_map<std::wstring, TeamSeasonStats>& teamStats,
		std::vector<FixtureService::Fixture>& fixtures,
		CalendarState& calendar,
		PersonalStats& personalStats,
		std::unordered_set<std::wstring>& storyFlags
	);

	bool GetAutosavePreview(int index, std::wstring& playerName, int& week);
	bool DeleteAutosave(int index);

	// Local "dd/mm/yyyy hh:mm" of when this autosave was written, or L"" if it doesn't exist.
	std::wstring GetAutosaveTimeLabel(int index);
}
#pragma once
#include "PlayerData.h"
#include "FixtureService.h"
#include <string>
#include <unordered_map>
#include <vector>

namespace SaveGameService
{
	constexpr int MaxSaveSlots = 3;

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
		PersonalStats const& personalStats = {}
	);

	bool LoadFromSlot(
		int slot,
		PlayerData& player,
		int& currentWeek,
		std::wstring& lastChoice,
		std::unordered_map<std::wstring, TeamSeasonStats>& teamStats,
		std::vector<FixtureService::Fixture>& fixtures,
		CalendarState& calendar,
		PersonalStats& personalStats
	);

	bool GetSavePreview(int slot, std::wstring& playerName, int& week);
	bool DeleteSlot(int slot);
}
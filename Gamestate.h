#pragma once
#include "PlayerData.h"
#include "FixtureService.h"
#include "SaveGameService.h"
#include <unordered_map>
#include <string>
#include <vector>

enum class DayPhase
{
	Monday,
	Tuesday,
	Wednesday,
	Thursday,
	Friday,
	Saturday,
	Sunday
};

// Lightweight calendar date - deliberately not std::chrono, to match the
// plain-struct style used elsewhere in the project (e.g. FixtureService::Fixture).
struct SimpleDate
{
	int Year = 2026;
	int Month = 3;
	int Day = 27;

	bool operator<(const SimpleDate& other) const
	{
		if (Year != other.Year) return Year < other.Year;
		if (Month != other.Month) return Month < other.Month;
		return Day < other.Day;
	}
	bool operator>=(const SimpleDate& other) const { return !(*this < other); }
	bool operator==(const SimpleDate& other) const
	{
		return Year == other.Year && Month == other.Month && Day == other.Day;
	}
};

struct GameState
{
	static PlayerData CurrentPlayer;
	static int CurrentWeek;
	static std::wstring LastChoice;
	static std::unordered_map<std::wstring, SaveGameService::TeamSeasonStats> TeamStats;
	static std::vector<FixtureService::Fixture> Fixtures;
	static std::unordered_map<std::wstring, int> XFactorStatModifiers;

	// Season calendar - source of truth for the day-by-day loop
	static DayPhase CurrentDay;
	static SimpleDate CurrentDate;
	static SimpleDate SeasonStartDate;
	static SimpleDate SeasonEndDate;
};
#pragma once
#include <string>
#include <vector>
#include <unordered_map>

namespace FixtureService
{
	struct Fixture
	{
		int Round;
		std::wstring HomeClub;
		std::wstring AwayClub;
		bool Played = false;
		int HomeScore = 0;
		int AwayScore = 0;

		// Empty for a normal home-and-away round. Non-empty ("Qualifying
		// Final", "Elimination Final", "Preliminary Final", "Grand Final",
		// or the internal "Season Over" marker) identifies a finals fixture.
		std::wstring FinalsLabel;
	};

	// Which finals system a league uses.
	//   Top4  - McIntyre Final Four (local leagues, Talent League)
	//   Top5  - "Final five" (SANFL / WAFL)
	//   Top10 - Wildcard Round + final eight (AFL / VFL from 2026)
	enum class FinalsSystem { Top4, Top5, Top10 };

	// How a league's season is laid out. Defaults = the original behaviour
	// (full double round-robin, top-4 finals, ~27 March start).
	struct SeasonPlan
	{
		int RegularRounds = 0;   // 0 = full double round-robin
		int BreakWeeks = 0;      // empty competition weeks spread through the home-and-away season
		FinalsSystem Finals = FinalsSystem::Top4;
		int StartMonth = 3;      // season opens on the first Friday on/after this date
		int StartDay = 27;
	};
	SeasonPlan SeasonPlanForLeague(const std::wstring& league);
	int FinalsWeeksFor(FinalsSystem format);   // 3 / 4 / 5
	int ClubsNeededFor(FinalsSystem format);   // 4 / 5 / 10

	// Generates a double round-robin fixture list. Uses the circle method;
	// if clubs.size() is odd, a "BYE" entry is inserted and omitted from
	// the resulting fixture list.
	//   maxRounds > 0 cuts the list off after that many rounds (leg one
	//     first, then the start of the reverse leg), so every club still
	//     plays the same number of games. 0 = full double round-robin.
	//   breakWeeks inserts that many empty round numbers (competition
	//     bye weeks), spread evenly through the season.
	std::vector<Fixture> GenerateDoubleRoundRobin(
		const std::vector<std::wstring>& clubs,
		int startWeek,
		int maxRounds = 0,
		int breakWeeks = 0);

	// Top-4 McIntyre Final Four system: Qualifying Final (1v2) and
	// Elimination Final (3v4) in week one. QF winner earns a bye straight
	// to the Grand Final; QF loser gets a second chance in the Preliminary
	// Final against the EF winner; EF loser is eliminated.
	std::vector<Fixture> GenerateFinalsWeek1(
		const std::vector<std::wstring>& top4,
		int round);

	// Final fixtures from GenerateFinalsWeek1.
	Fixture GenerateFinalsWeek2(
		const std::vector<Fixture>& week1Finals,
		int round);

	// Grand Final: QF winner (home) vs Preliminary Final winner (away).
	Fixture GenerateGrandFinal(
		const std::vector<Fixture>& week1Finals,
		const Fixture& prelimFinal,
		int round);

	// Next finals week's fixtures for the given format, given everything played so far
	std::vector<Fixture> GenerateNextFinalsWeek(
		FinalsSystem format,
		const std::vector<std::wstring>& ladder,
		const std::vector<Fixture>& fixtures,
		int nextRound);

	// Reads [Tier],StartWeek,ByeRounds,FinalsWeeks,FinalsFormat from CSV.
	struct SeasonStructure
	{
		int StartWeek = 1;
		int ByeRounds = 0;
		int FinalsWeeks = 0;
		std::wstring FinalsFormat = L"None";
	};
	SeasonStructure LoadSeasonStructure(const std::wstring& csvPath, const std::wstring& tier);

	// Persistence, mirrors the [TeamStats] section pattern.
	void SaveFixtures(std::wofstream& out, const std::vector<Fixture>& fixtures);
	std::vector<Fixture> LoadFixtures(std::wifstream& in);
}
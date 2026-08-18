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

	// Generates a full home-and-away (double round-robin) fixture list.
	// Uses the circle method; if clubs.size() is odd, a "BYE" entry is
	// inserted and omitted from the resulting fixture list.
	std::vector<Fixture> GenerateDoubleRoundRobin(
		const std::vector<std::wstring>& clubs,
		int startWeek);

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
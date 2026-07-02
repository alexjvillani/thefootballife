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
    };

    // Generates a full home-and-away (double round-robin) fixture list.
    // Uses the circle method; if clubs.size() is odd, a "BYE" entry is
    // inserted and omitted from the resulting fixture list.
    std::vector<Fixture> GenerateDoubleRoundRobin(
        const std::vector<std::wstring>& clubs,
        int startWeek);

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
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
        std::vector<FixtureService::Fixture> const& fixtures = {}
    );

    bool LoadFromSlot(
        int slot,
        PlayerData& player,
        int& currentWeek,
        std::wstring& lastChoice,
        std::unordered_map<std::wstring, TeamSeasonStats>& teamStats,
        std::vector<FixtureService::Fixture>& fixtures
    );

    bool GetSavePreview(int slot, std::wstring& playerName, int& week);
    bool DeleteSlot(int slot);
}
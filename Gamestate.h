#pragma once
#include "PlayerData.h"
#include "FixtureService.h"
#include "SaveGameService.h"
#include <unordered_map>
#include <string>
#include <vector>

struct GameState
{
    static PlayerData CurrentPlayer;
    static int CurrentWeek;
    static std::wstring LastChoice;
    static std::unordered_map<std::wstring, SaveGameService::TeamSeasonStats> TeamStats;
    static std::vector<FixtureService::Fixture> Fixtures;
};
#pragma once
#include "PlayerData.h"
#include "SaveGameService.h"
#include <unordered_map>
#include <string>

struct GameState
{
    static PlayerData CurrentPlayer;
    static int CurrentWeek;
    static std::wstring LastChoice;
    static std::unordered_map<std::wstring, SaveGameService::TeamSeasonStats> TeamStats;
};
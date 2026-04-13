#pragma once
#include "PlayerData.h"

struct GameState
{
    static PlayerData CurrentPlayer;
    static int CurrentWeek;
    static std::wstring LastChoice;
};

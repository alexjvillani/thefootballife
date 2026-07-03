#include "pch.h"
#include "GameState.h"

PlayerData GameState::CurrentPlayer;
int GameState::CurrentWeek = 1;
std::wstring GameState::LastChoice = L"No action chosen yet.";
std::unordered_map<std::wstring, SaveGameService::TeamSeasonStats> GameState::TeamStats;
std::vector<FixtureService::Fixture> GameState::Fixtures;

#pragma once
#include <string>

struct PlayerData
{
    std::wstring firstName;
    std::wstring lastName;
    std::wstring position;
    std::wstring foot;
    std::wstring number;
    std::wstring team;
    std::wstring originalTeam;
    std::wstring originalTeamSuburb;
    std::wstring originalTeamPrimaryColour;
    std::wstring originalTeamSecondaryColour;
    std::wstring originalTeamHomeGround;
    std::wstring originalTeamLeague;
    int originalTeamReputation{ 50 };
    std::wstring state;
    std::wstring schoolType;
    std::wstring region;
    std::wstring familySituation;
    std::wstring finances;
    int heightCm{ 0 };
    int weightKg{ 0 };
    int potentialHeightCm{ 0 };
    int distanceToClubKm{ 0 };
    std::wstring mentalityXFactor;
    std::wstring physicalXFactor;
    std::wstring weaknesses;
    std::wstring profileImagePath;
};
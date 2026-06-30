#include "pch.h"
#include "SaveGameService.h"

#include <winrt/Windows.Storage.h>
#include <fstream>
#include <string>
#include <unordered_map>
#include <filesystem>
#include <ShlObj.h>
#include <sstream>

#pragma comment(lib, "Shell32.lib")
#pragma comment(lib, "Ole32.lib")

using namespace winrt::Windows::Storage;
namespace fs = std::filesystem;

namespace
{
    bool TryParseInt(std::wstring const& value, int& output)
    {
        try
        {
            output = std::stoi(value);
            return true;
        }
        catch (...)
        {
            return false;
        }
    }
}

namespace SaveGameService
{
    std::wstring GetSaveFolder()
    {
        PWSTR path = nullptr;

        HRESULT hr = SHGetKnownFolderPath(FOLDERID_Documents, 0, NULL, &path);
        if (FAILED(hr))
        {
            return L"";
        }

        std::filesystem::path docPath(path);
        CoTaskMemFree(path);

        docPath /= L"The Football Life"; 

        // ensure folder exists
        std::filesystem::create_directories(docPath);

        return docPath.wstring();
    }

    std::wstring GetSaveSlotPath(int slot)
    {
        std::wstring path = GetSaveFolder();

        path += L"\\career_save_slot";
        path += std::to_wstring(slot);
        path += L".txt";

        return path;
    }

    bool SlotExists(int slot)
    {
        if (slot < 1 || slot > MaxSaveSlots)
        {
            return false;
        }

        std::wifstream file(GetSaveSlotPath(slot));
        return file.is_open();
    }

    int FindFirstAvailableSlot()
    {
        for (int slot = 1; slot <= MaxSaveSlots; ++slot)
        {
            if (!SlotExists(slot))
            {
                return slot;
            }
        }

        return 1;
    }

    bool SaveToSlot(
        int slot,
        PlayerData const& player,
        int currentWeek,
        std::wstring const& lastChoice,
        std::unordered_map<std::wstring, TeamSeasonStats> const& teamStats
    )
    {
        if (slot < 1 || slot > MaxSaveSlots) return false;

        std::wofstream file(GetSaveSlotPath(slot));
        if (!file.is_open()) return false;

        file << L"FirstName=" << player.firstName << L"\n";
        file << L"LastName=" << player.lastName << L"\n";
        file << L"Position=" << player.position << L"\n";
        file << L"Foot=" << player.foot << L"\n";
        file << L"Number=" << player.number << L"\n";
        file << L"Team=" << player.team << L"\n";
        file << L"OriginalTeam=" << player.originalTeam << L"\n";
        file << L"OriginalTeamSuburb=" << player.originalTeamSuburb << L"\n";
        file << L"OriginalTeamPrimaryColour=" << player.originalTeamPrimaryColour << L"\n";
        file << L"OriginalTeamSecondaryColour=" << player.originalTeamSecondaryColour << L"\n";
        file << L"OriginalTeamHomeGround=" << player.originalTeamHomeGround << L"\n";
        file << L"OriginalTeamLeague=" << player.originalTeamLeague << L"\n";
        file << L"OriginalTeamReputation=" << player.originalTeamReputation << L"\n";
        file << L"State=" << player.state << L"\n";
        file << L"SchoolType=" << player.schoolType << L"\n";
        file << L"Region=" << player.region << L"\n";
        file << L"FamilySituation=" << player.familySituation << L"\n";
        file << L"Finances=" << player.finances << L"\n";
        file << L"HeightCm=" << player.heightCm << L"\n";
        file << L"WeightKg=" << player.weightKg << L"\n";
        file << L"PotentialHeightCm=" << player.potentialHeightCm << L"\n";
        file << L"DistanceToClubKm=" << player.distanceToClubKm << L"\n";
        file << L"MentalityXFactor=" << player.mentalityXFactor << L"\n";
        file << L"PhysicalXFactor=" << player.physicalXFactor << L"\n";
        file << L"Weaknesses=" << player.weaknesses << L"\n";
        file << L"ProfileImagePath=" << player.profileImagePath << L"\n";
        file << L"CurrentWeek=" << currentWeek << L"\n";
        file << L"LastChoice=" << lastChoice << L"\n";

        // Team stats section
        if (!teamStats.empty())
        {
            file << L"[TeamStats]\n";
            for (auto const& [clubName, s] : teamStats)
            {
                file << clubName << L"="
                    << s.wins << L","
                    << s.losses << L","
                    << s.draws << L","
                    << s.pointsFor << L","
                    << s.pointsAgainst << L"\n";
            }
        }

        return true;
    }

    bool LoadFromSlot(
        int slot,
        PlayerData& player,
        int& currentWeek,
        std::wstring& lastChoice,
        std::unordered_map<std::wstring, TeamSeasonStats>& teamStats
    )
    {
        if (slot < 1 || slot > MaxSaveSlots) return false;

        std::wifstream file(GetSaveSlotPath(slot));
        if (!file.is_open()) return false;

        std::unordered_map<std::wstring, std::wstring> values;
        teamStats.clear();

        bool inTeamStats = false;
        std::wstring line;
        while (std::getline(file, line))
        {
            if (line == L"[TeamStats]") { inTeamStats = true; continue; }

            if (inTeamStats)
            {
                // Format: ClubName=W,L,D,PF,PA
                size_t eq = line.find(L'=');
                if (eq == std::wstring::npos) continue;
                std::wstring club = line.substr(0, eq);
                std::wstring data = line.substr(eq + 1);

                TeamSeasonStats s;
                std::wistringstream ss(data);
                std::wstring tok;
                int idx = 0;
                while (std::getline(ss, tok, L','))
                {
                    try {
                        int v = std::stoi(tok);
                        if (idx == 0) s.wins = v;
                        else if (idx == 1) s.losses = v;
                        else if (idx == 2) s.draws = v;
                        else if (idx == 3) s.pointsFor = v;
                        else if (idx == 4) s.pointsAgainst = v;
                    }
                    catch (...) {}
                    ++idx;
                }
                teamStats[club] = s;
                continue;
            }

            size_t sep = line.find(L'=');
            if (sep == std::wstring::npos) continue;
            values[line.substr(0, sep)] = line.substr(sep + 1);
        }

        player.firstName = values[L"FirstName"];
        player.lastName = values[L"LastName"];
        player.position = values[L"Position"];
        player.foot = values[L"Foot"];
        player.number = values[L"Number"];
        player.team = values[L"Team"];
        player.state = values[L"State"];
        player.originalTeam = values[L"OriginalTeam"];
        player.originalTeamSuburb = values[L"OriginalTeamSuburb"];
        player.originalTeamPrimaryColour = values[L"OriginalTeamPrimaryColour"];
        player.originalTeamSecondaryColour = values[L"OriginalTeamSecondaryColour"];
        player.originalTeamHomeGround = values[L"OriginalTeamHomeGround"];
        player.originalTeamLeague = values[L"OriginalTeamLeague"];
        player.schoolType = values[L"SchoolType"];
        player.region = values[L"Region"];
        player.familySituation = values[L"FamilySituation"];
        player.finances = values[L"Finances"];
        player.mentalityXFactor = values[L"MentalityXFactor"];
        player.physicalXFactor = values[L"PhysicalXFactor"];
        player.weaknesses = values[L"Weaknesses"];
        player.profileImagePath = values[L"ProfileImagePath"];

        if (!TryParseInt(values[L"HeightCm"], player.heightCm))         player.heightCm = 0;
        if (!TryParseInt(values[L"WeightKg"], player.weightKg))          player.weightKg = 0;
        if (!TryParseInt(values[L"PotentialHeightCm"], player.potentialHeightCm)) player.potentialHeightCm = 0;
        if (!TryParseInt(values[L"DistanceToClubKm"], player.distanceToClubKm))  player.distanceToClubKm = 0;
        if (!TryParseInt(values[L"OriginalTeamReputation"], player.originalTeamReputation)) player.originalTeamReputation = 50;
        if (!TryParseInt(values[L"CurrentWeek"], currentWeek))              currentWeek = 1;

        lastChoice = values[L"LastChoice"];
        if (lastChoice.empty()) lastChoice = L"No action chosen yet.";

        return true;
    }

    bool GetSavePreview(int slot, std::wstring& playerName, int& week)
    {
        if (slot < 1 || slot > MaxSaveSlots)
        {
            return false;
        }

        std::wifstream file(GetSaveSlotPath(slot));
        if (!file.is_open())
        {
            return false;
        }

        std::unordered_map<std::wstring, std::wstring> values;
        std::wstring line;
        while (std::getline(file, line))
        {
            size_t separator = line.find(L'=');
            if (separator == std::wstring::npos)
            {
                continue;
            }

            values[line.substr(0, separator)] = line.substr(separator + 1);
        }

        std::wstring firstName = values[L"FirstName"];
        std::wstring lastName = values[L"LastName"];

        playerName = firstName;
        if (!firstName.empty() && !lastName.empty())
        {
            playerName += L" ";
        }
        playerName += lastName;

        if (playerName.empty())
        {
            playerName = L"Unknown Player";
        }

        if (!TryParseInt(values[L"CurrentWeek"], week))
        {
            week = 1;
        }

        return true;
    }

    bool DeleteSlot(int slot)
    {
        if (slot < 1 || slot > MaxSaveSlots)
        {
            return false;
        }

        std::wstring path = GetSaveSlotPath(slot);

        if (!fs::exists(path))
        {
            return false;
        }

        return fs::remove(path);
    }
}
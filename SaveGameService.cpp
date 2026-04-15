#include "pch.h"
#include "SaveGameService.h"

#include <winrt/Windows.Storage.h>
#include <fstream>
#include <string>
#include <unordered_map>

using namespace winrt::Windows::Storage;

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
    std::wstring GetSaveSlotPath(int slot)
    {
        std::wstring path = ApplicationData::Current().LocalFolder().Path().c_str();
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
        std::wstring const& lastChoice
    )
    {
        if (slot < 1 || slot > MaxSaveSlots)
        {
            return false;
        }

        std::wofstream file(GetSaveSlotPath(slot));
        if (!file.is_open())
        {
            return false;
        }

        file << L"FirstName=" << player.firstName << L"\n";
        file << L"LastName=" << player.lastName << L"\n";
        file << L"Position=" << player.position << L"\n";
        file << L"Foot=" << player.foot << L"\n";
        file << L"Number=" << player.number << L"\n";
        file << L"Team=" << player.team << L"\n";
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

        return true;
    }

    bool LoadFromSlot(
        int slot,
        PlayerData& player,
        int& currentWeek,
        std::wstring& lastChoice
    )
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

        player.firstName = values[L"FirstName"];
        player.lastName = values[L"LastName"];
        player.position = values[L"Position"];
        player.foot = values[L"Foot"];
        player.number = values[L"Number"];
        player.team = values[L"Team"];
        player.state = values[L"State"];
        player.schoolType = values[L"SchoolType"];
        player.region = values[L"Region"];
        player.familySituation = values[L"FamilySituation"];
        player.finances = values[L"Finances"];
        player.mentalityXFactor = values[L"MentalityXFactor"];
        player.physicalXFactor = values[L"PhysicalXFactor"];
        player.weaknesses = values[L"Weaknesses"];
        player.profileImagePath = values[L"ProfileImagePath"];

        if (!TryParseInt(values[L"HeightCm"], player.heightCm))
        {
            player.heightCm = 0;
        }

        if (!TryParseInt(values[L"WeightKg"], player.weightKg))
        {
            player.weightKg = 0;
        }

        if (!TryParseInt(values[L"PotentialHeightCm"], player.potentialHeightCm))
        {
            player.potentialHeightCm = 0;
        }

        if (!TryParseInt(values[L"DistanceToClubKm"], player.distanceToClubKm))
        {
            player.distanceToClubKm = 0;
        }

        if (!TryParseInt(values[L"CurrentWeek"], currentWeek))
        {
            currentWeek = 1;
        }

        lastChoice = values[L"LastChoice"];
        if (lastChoice.empty())
        {
            lastChoice = L"No action chosen yet.";
        }

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
}
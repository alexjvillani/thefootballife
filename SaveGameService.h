#pragma once

#include "PlayerData.h"

namespace SaveGameService
{
    constexpr int MaxSaveSlots = 3;

    std::wstring GetSaveSlotPath(int slot);
    bool SlotExists(int slot);
    int FindFirstAvailableSlot();

    bool SaveToSlot(
        int slot,
        PlayerData const& player,
        int currentWeek,
        std::wstring const& lastChoice
    );

    bool LoadFromSlot(
        int slot,
        PlayerData& player,
        int& currentWeek,
        std::wstring& lastChoice
    );
    
    bool DeleteSlot(int slot);
    bool GetSavePreview(int slot, std::wstring& playerName, int& week);


}
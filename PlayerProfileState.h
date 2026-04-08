#pragma once

#include <winrt/base.h>

namespace thefootballife
{
    struct PlayerProfileState
    {
        winrt::hstring firstName;
        winrt::hstring lastName;
        winrt::hstring position;
        winrt::hstring preferredFoot;
        winrt::hstring number;
        winrt::hstring team;
        winrt::hstring state;
        winrt::hstring school;
        winrt::hstring region;

        winrt::hstring height;
        winrt::hstring weightKg;
        winrt::hstring potentialHeight;
        winrt::hstring familySituation;
        winrt::hstring finances;
        winrt::hstring schoolQuality;
        winrt::hstring distanceToClubKm;

        winrt::hstring mentalityXFactor;
        winrt::hstring physicalXFactor;
        winrt::hstring weaknesses;
    };

    inline PlayerProfileState g_playerProfileState{};
}

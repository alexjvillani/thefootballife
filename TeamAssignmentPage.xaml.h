#pragma once
#include "TeamAssignmentPage.g.h"
#include <vector>

namespace winrt::thefootballife::implementation
{
    struct TeamAssignmentPage : TeamAssignmentPageT<TeamAssignmentPage>
    {
        TeamAssignmentPage();
        winrt::hstring PageTitle();
        void PageTitle(winrt::hstring const& value);
        void RegenerateButton_Click(
            winrt::Windows::Foundation::IInspectable const& sender,
            winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        void ConfirmTeamButton_Click(
            winrt::Windows::Foundation::IInspectable const& sender,
            winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);

    private:
        struct TeamProfile
        {
            std::wstring name;
            std::wstring suburb;
            std::wstring primaryColour;
            std::wstring secondaryColour;
            std::wstring homeGround;
            std::wstring league;
            int          baseDistanceKm{ 0 };
            int          reputation{ 50 };
        };

        std::vector<TeamProfile> BuildTeamsForState(std::wstring const& state);
        std::wstring             ResolvePlayerState() const;
        void                     GenerateTeamAssignment(bool regenerateNames);
        void                     RenderTeamsList();

    private:
        winrt::hstring           m_pageTitle{ L"Local Club Assignment" };
        std::vector<TeamProfile> m_stateTeams;
        TeamProfile              m_assignedTeam;
        bool                     m_hasAssignedTeam{ false };
    };
}

namespace winrt::thefootballife::factory_implementation
{
    struct TeamAssignmentPage : TeamAssignmentPageT<TeamAssignmentPage, implementation::TeamAssignmentPage>
    {
    };
}
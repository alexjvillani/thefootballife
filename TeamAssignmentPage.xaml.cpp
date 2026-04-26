#include "pch.h"
#include "TeamAssignmentPage.xaml.h"
#if __has_include("TeamAssignmentPage.g.cpp")
#include "TeamAssignmentPage.g.cpp"
#endif

#include "GameState.h"
#include <algorithm>
#include <random>
#include <limits>
#include <winrt/Windows.UI.Xaml.Interop.h>

using namespace winrt;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Media;

namespace
{
    std::wstring BuildClubName(std::wstring const& suburb, std::wstring const& mascot)
    {
        return suburb + L" " + mascot;
    }
}

namespace winrt::thefootballife::implementation
{
    TeamAssignmentPage::TeamAssignmentPage()
    {
        InitializeComponent();
        GenerateTeamAssignment(true);
    }

    hstring TeamAssignmentPage::PageTitle()
    {
        return m_pageTitle;
    }

    void TeamAssignmentPage::PageTitle(hstring const& value)
    {
        m_pageTitle = value;
    }

    std::wstring TeamAssignmentPage::ResolvePlayerState() const
    {
        std::wstring state = GameState::CurrentPlayer.state;
        if (state.empty())
        {
            return L"Victoria";
        }

        return state;
    }

    std::vector<TeamAssignmentPage::TeamProfile> TeamAssignmentPage::BuildTeamsForState(std::wstring const& state)
    {
        std::vector<std::wstring> suburbs;
        if (state == L"Victoria")
            suburbs = { L"Richmond", L"Footscray", L"Dandenong", L"Geelong", L"Ballarat", L"Bendigo", L"Frankston", L"Carlton", L"Werribee", L"Shepparton" };
        else if (state == L"New South Wales")
            suburbs = { L"Parramatta", L"Newtown", L"Penrith", L"Wollongong", L"Newcastle", L"Bathurst", L"Albury", L"Dubbo", L"Gosford", L"Campbelltown" };
        else if (state == L"Queensland")
            suburbs = { L"South Brisbane", L"Gold Coast", L"Cairns", L"Townsville", L"Toowoomba", L"Logan", L"Ipswich", L"Mackay", L"Rockhampton", L"Bundaberg" };
        else if (state == L"South Australia")
            suburbs = { L"Norwood", L"Glenelg", L"Port Adelaide", L"Prospect", L"Mawson Lakes", L"Mount Gambier", L"Whyalla", L"Victor Harbor", L"Murray Bridge", L"Elizabeth" };
        else if (state == L"Western Australia")
            suburbs = { L"Fremantle", L"Joondalup", L"Subiaco", L"Midland", L"Bunbury", L"Mandurah", L"Kalgoorlie", L"Albany", L"Geraldton", L"Rockingham" };
        else if (state == L"Tasmania")
            suburbs = { L"Hobart", L"Launceston", L"Devonport", L"Burnie", L"Kingston", L"Glenorchy", L"Ulverstone", L"Richmond", L"Scottsdale", L"Huonville" };
        else if (state == L"Northern Territory")
            suburbs = { L"Darwin", L"Palmerston", L"Alice Springs", L"Katherine", L"Nhulunbuy", L"Tennant Creek", L"Jabiru", L"Humpty Doo", L"Casuarina", L"Nightcliff" };
        else
            suburbs = { L"Canberra", L"Belconnen", L"Tuggeranong", L"Woden", L"Gungahlin", L"Queanbeyan", L"Fyshwick", L"Narrabundah", L"Dickson", L"Kambah" };

        std::vector<std::wstring> mascots =
        {
            L"Falcons", L"Storm", L"Rangers", L"Lions", L"Titans", L"Wolves", L"Roos", L"Jets", L"Sharks", L"Panthers", L"Eagles", L"Demons"
        };

        std::vector<std::pair<std::wstring, std::wstring>> colours =
        {
            { L"Navy", L"Gold" }, { L"Maroon", L"White" }, { L"Black", L"Red" }, { L"Royal Blue", L"Silver" },
            { L"Forest Green", L"White" }, { L"Purple", L"Gold" }, { L"Teal", L"Black" }, { L"Crimson", L"White" },
            { L"Sky Blue", L"Navy" }, { L"Orange", L"Charcoal" }, { L"Emerald", L"Gold" }, { L"Burgundy", L"Cream" }
        };

        std::random_device rd;
        std::mt19937 gen(rd());
        std::shuffle(mascots.begin(), mascots.end(), gen);
        std::shuffle(colours.begin(), colours.end(), gen);

        std::vector<TeamProfile> teams;
        teams.reserve(suburbs.size());

        for (size_t i = 0; i < suburbs.size(); ++i)
        {
            TeamProfile team;
            team.suburb = suburbs[i];
            team.name = BuildClubName(suburbs[i], mascots[i % mascots.size()]);
            team.primaryColour = colours[i % colours.size()].first;
            team.secondaryColour = colours[i % colours.size()].second;
            team.baseDistanceKm = 4 + static_cast<int>(i) * 6;
            teams.push_back(team);
        }

        return teams;
    }

    void TeamAssignmentPage::RenderTeamsList()
    {
        TeamsListPanel().Children().Clear();

        for (auto const& team : m_stateTeams)
        {
            Border item;
            item.Background(SolidColorBrush(winrt::Windows::UI::ColorHelper::FromArgb(255, 38, 38, 38)));
            item.CornerRadius(CornerRadius{ 8, 8, 8, 8 });
            item.Padding(Thickness{ 10, 10, 10, 10 });

            StackPanel row;
            row.Spacing(4);

            TextBlock title;
            title.Text(hstring(team.name));
            title.Foreground(SolidColorBrush(winrt::Windows::UI::Colors::White()));
            title.FontWeight(Windows::UI::Text::FontWeights::SemiBold());

            TextBlock meta;
            std::wstring metaText = L"Location: " + team.suburb +
                L"  •  Club profile distance: " + std::to_wstring(team.baseDistanceKm) + L" km" +
                L"  •  Colours: " + team.primaryColour + L" / " + team.secondaryColour;
            meta.Text(hstring(metaText));
            meta.Foreground(SolidColorBrush(winrt::Windows::UI::ColorHelper::FromArgb(255, 210, 210, 210)));
            meta.TextWrapping(TextWrapping::Wrap);

            row.Children().Append(title);
            row.Children().Append(meta);
            item.Child(row);
            TeamsListPanel().Children().Append(item);
        }
    }

    void TeamAssignmentPage::GenerateTeamAssignment(bool regenerateNames)
    {
        std::wstring state = ResolvePlayerState();
        if (regenerateNames || m_stateTeams.empty())
        {
            m_stateTeams = BuildTeamsForState(state);
        }

        int targetDistance = GameState::CurrentPlayer.distanceToClubKm;
        int bestDiff = std::numeric_limits<int>::max();
        std::vector<size_t> candidates;

        for (size_t i = 0; i < m_stateTeams.size(); ++i)
        {
            int diff = std::abs(targetDistance - m_stateTeams[i].baseDistanceKm);
            if (diff < bestDiff)
            {
                bestDiff = diff;
                candidates.clear();
                candidates.push_back(i);
            }
            else if (diff == bestDiff)
            {
                candidates.push_back(i);
            }
        }

        size_t assignedIndex = 0;
        if (!candidates.empty())
        {
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<size_t> dist(0, candidates.size() - 1);
            assignedIndex = candidates[dist(gen)];
        }

        m_assignedTeam = m_stateTeams[assignedIndex];
        m_hasAssignedTeam = true;

        HeaderText().Text(
            L"Generated 10 clubs in " + hstring(state) +
            L" and matched the closest profile to your distance from club: " + to_hstring(targetDistance) + L" km."
        );

        StateTeamsTitleText().Text(L"10 generated clubs for " + hstring(state));
        AssignedTeamText().Text(hstring(m_assignedTeam.name));
        AssignedSuburbText().Text(L"Home suburb: " + hstring(m_assignedTeam.suburb));
        AssignedDistanceText().Text(
            L"Distance match: profile " + to_hstring(m_assignedTeam.baseDistanceKm) +
            L" km (player: " + to_hstring(targetDistance) + L" km)"
        );
        AssignedColourText().Text(
            L"Two-tone colours: " + hstring(m_assignedTeam.primaryColour) + L" / " + hstring(m_assignedTeam.secondaryColour)
        );

        RenderTeamsList();
    }

    void TeamAssignmentPage::RegenerateButton_Click(IInspectable const&, RoutedEventArgs const&)
    {
        GenerateTeamAssignment(true);
    }

    void TeamAssignmentPage::ConfirmTeamButton_Click(IInspectable const&, RoutedEventArgs const&)
    {
        if (!m_hasAssignedTeam)
        {
            return;
        }

        GameState::CurrentPlayer.originalTeam = m_assignedTeam.name;
        GameState::CurrentPlayer.originalTeamSuburb = m_assignedTeam.suburb;
        GameState::CurrentPlayer.originalTeamPrimaryColour = m_assignedTeam.primaryColour;
        GameState::CurrentPlayer.originalTeamSecondaryColour = m_assignedTeam.secondaryColour;

        Frame().Navigate(
            winrt::Windows::UI::Xaml::Interop::TypeName{
                L"thefootballife.CareerHubPage",
                winrt::Windows::UI::Xaml::Interop::TypeKind::Custom
            }
        );
    }
}

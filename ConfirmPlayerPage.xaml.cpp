#include "pch.h"
#include "ConfirmPlayerPage.xaml.h"
#if __has_include("ConfirmPlayerPage.g.cpp")
#include "ConfirmPlayerPage.g.cpp"
#endif

#include "GameState.h"
#include <winrt/Windows.Foundation.h>
#include <winrt/Microsoft.UI.Xaml.Media.Imaging.h>
#include <winrt/Windows.UI.Xaml.Interop.h>

using namespace winrt;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Media::Imaging;
using namespace Windows::Foundation;

namespace
{
    winrt::hstring FormatHeightFeet(int totalCm)
    {
        double totalInches = static_cast<double>(totalCm) / 2.54;
        int roundedInches = static_cast<int>(totalInches + 0.5);
        int feet = roundedInches / 12;
        int inches = roundedInches % 12;

        return winrt::to_hstring(feet) + L"'" + winrt::to_hstring(inches) + L"\"";
    }
}

namespace winrt::thefootballife::implementation
{
    ConfirmPlayerPage::ConfirmPlayerPage()
    {
        InitializeComponent();
        LoadPreviewData();
    }

    hstring ConfirmPlayerPage::PageTitle()
    {
        return m_pageTitle;
    }

    void ConfirmPlayerPage::PageTitle(hstring const& value)
    {
        m_pageTitle = value;
    }

    void ConfirmPlayerPage::LoadPreviewData()
    {
        auto const& player = GameState::CurrentPlayer;

        if (!player.profileImagePath.empty())
        {
            BitmapImage bitmap;
            bitmap.UriSource(Uri(player.profileImagePath));
            ProfileImage().Source(bitmap);
        }

        NameText().Text(L"Name: " + hstring(player.firstName + L" " + player.lastName));
        PositionText().Text(L"Position: " + hstring(player.position));
        FootText().Text(L"Preferred Foot: " + hstring(player.foot));
        NumberText().Text(L"Number: " + hstring(player.number));
        TeamText().Text(L"Supported Team: " + hstring(player.team));
        StateText().Text(L"State of Origin: " + hstring(player.state));
        SchoolText().Text(L"School Type: " + hstring(player.schoolType));
        RegionText().Text(L"Region: " + hstring(player.region));
        FamilyText().Text(L"Family Situation: " + hstring(player.familySituation));

        HeightWeightText().Text(
            L"Height / Weight: " +
            FormatHeightFeet(player.heightCm) +
            L" / " +
            to_hstring(player.weightKg) +
            L" kg"
        );

        PotentialHeightText().Text(
            L"Potential Height: " + FormatHeightFeet(player.potentialHeightCm)
        );

        DistanceText().Text(
            L"Distance to Club: " + to_hstring(player.distanceToClubKm) + L" km"
        );

        FinancesText().Text(L"Finances: " + hstring(player.finances));

        if (player.mentalityXFactor.empty())
            MentalityText().Text(L"None selected");
        else
            MentalityText().Text(hstring(player.mentalityXFactor));

        if (player.physicalXFactor.empty())
            PhysicalText().Text(L"None selected");
        else
            PhysicalText().Text(hstring(player.physicalXFactor));

        if (player.weaknesses.empty())
            WeaknessesText().Text(L"None selected");
        else
            WeaknessesText().Text(hstring(player.weaknesses));

        OverallBlurbText().Text(
            L"A promising young player with strong upside and a unique development path ahead."
        );

        ScoutViewText().Text(
            L"An intriguing prospect with genuine upside if development is handled well."
        );
    }

    void ConfirmPlayerPage::BackButton_Click(IInspectable const&, RoutedEventArgs const&)
    {
        if (Frame().CanGoBack())
        {
            Frame().GoBack();
        }
    }

    void ConfirmPlayerPage::StartCareerButton_Click(IInspectable const&, RoutedEventArgs const&)
    {
        ContentDialog dialog;
        dialog.Title(box_value(L"Career Starting"));
        dialog.Content(box_value(L"Next step: navigate to CareerHubPage and load the player into the weekly career loop."));
        dialog.CloseButtonText(L"OK");
        dialog.XamlRoot(this->XamlRoot());
        dialog.ShowAsync();
    }
}
#include "pch.h"
#include "ConfirmPlayerPage.xaml.h"
#if __has_include("ConfirmPlayerPage.g.cpp")
#include "ConfirmPlayerPage.g.cpp"
#endif

#include <winrt/Windows.Foundation.h>
#include <winrt/Microsoft.UI.Xaml.Media.Imaging.h>
#include <winrt/Windows.UI.Xaml.Interop.h>

using namespace winrt;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Media::Imaging;
using namespace Windows::Foundation;

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
        // Placeholder preview data for now.
        // Next step will be replacing these with real shared Player data.

        BitmapImage bitmap;
        bitmap.UriSource(Uri(L"ms-appx:///Assets/StoreLogo.png"));
        ProfileImage().Source(bitmap);

        NameText().Text(L"Name: Alex Villani");
        PositionText().Text(L"Position: Midfielder");
        FootText().Text(L"Preferred Foot: Right");
        NumberText().Text(L"Number: 9");
        TeamText().Text(L"Supported Team: Carlton");
        StateText().Text(L"State of Origin: Victoria");
        SchoolText().Text(L"School Type: Public School");
        RegionText().Text(L"Region: Metro");
        FamilyText().Text(L"Family Situation: Stable Home");
        HeightWeightText().Text(L"Height / Weight: 6'1\" / 82 kg");
        PotentialHeightText().Text(L"Potential Height: 6'3\"");
        DistanceText().Text(L"Distance to Club: 24 km");
        FinancesText().Text(L"Finances: Stable");

        MentalityText().Text(L"Composed");
        PhysicalText().Text(L"Explosive Speed");
        WeaknessesText().Text(L"Inconsistent Kicking, Low Endurance");

        OverallBlurbText().Text(L"A promising young player with strong upside and a unique development path ahead.");
        ScoutViewText().Text(L"An intriguing prospect with genuine upside if development is handled well.");
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
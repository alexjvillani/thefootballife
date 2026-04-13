#include "pch.h"
#include "MyCareerPage.xaml.h"
#if __has_include("MyCareerPage.g.cpp")
#include "MyCareerPage.g.cpp"
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

namespace winrt::thefootballife::implementation
{
    MyCareerPage::MyCareerPage()
    {
        InitializeComponent();
        LoadCareerData();
    }

    hstring MyCareerPage::PageTitle()
    {
        return m_pageTitle;
    }

    void MyCareerPage::PageTitle(hstring const& value)
    {
        m_pageTitle = value;
    }

    void MyCareerPage::LoadCareerData()
    {
        auto const& player = GameState::CurrentPlayer;

        if (!player.profileImagePath.empty())
        {
            BitmapImage bitmap;
            bitmap.UriSource(Uri(player.profileImagePath));
            ProfileImage().Source(bitmap);
        }

        PlayerNameText().Text(hstring(player.firstName + L" " + player.lastName));
        PlayerSummaryText().Text(
            L"A developing prospect building toward a bigger football future."
        );

        TeamText().Text(L"Supported Team: " + hstring(player.team));
        PositionText().Text(L"Position: " + hstring(player.position));

        if (player.mentalityXFactor.empty())
            MentalityText().Text(L"Mentality: None selected");
        else
            MentalityText().Text(L"Mentality: " + hstring(player.mentalityXFactor));

        if (player.physicalXFactor.empty())
            PhysicalText().Text(L"Physical: None selected");
        else
            PhysicalText().Text(L"Physical: " + hstring(player.physicalXFactor));

        if (player.weaknesses.empty())
            WeaknessesText().Text(L"Weaknesses: None selected");
        else
            WeaknessesText().Text(L"Weaknesses: " + hstring(player.weaknesses));

        WeekText().Text(L"Current Week: " + to_hstring(GameState::CurrentWeek));
    }

    void MyCareerPage::BackToCareerHubButton_Click(IInspectable const&, RoutedEventArgs const&)
    {
        Frame().Navigate(
            winrt::Windows::UI::Xaml::Interop::TypeName{
                L"thefootballife.CareerHubPage",
                winrt::Windows::UI::Xaml::Interop::TypeKind::Custom
            }
        );
    }
}

#include "pch.h"
#include "ConfirmPlayerPage.xaml.h"
#include "PlayerProfileState.h"
#if __has_include("ConfirmPlayerPage.g.cpp")
#include "ConfirmPlayerPage.g.cpp"
#endif

using namespace winrt;
using namespace Microsoft::UI::Xaml;

namespace winrt::thefootballife::implementation
{
    ConfirmPlayerPage::ConfirmPlayerPage()
    {
        InitializeComponent();
        LoadPlayerSummary();
    }

    void ConfirmPlayerPage::LoadPlayerSummary()
    {
        auto const& player = thefootballife::g_playerProfileState;

        std::wstring identity =
            L"Name: " + std::wstring(player.firstName) + L" " + std::wstring(player.lastName) +
            L"\nPosition: " + std::wstring(player.position) +
            L"\nPreferred Foot: " + std::wstring(player.preferredFoot) +
            L"\nNumber: " + std::wstring(player.number) +
            L"\nSupported Team: " + std::wstring(player.team) +
            L"\nState of Origin: " + std::wstring(player.state);

        std::wstring background =
            L"School Type: " + std::wstring(player.school) +
            L"\nRegion: " + std::wstring(player.region) +
            L"\nSchool Quality: " + std::wstring(player.schoolQuality) +
            L"\nFamily Situation: " + std::wstring(player.familySituation) +
            L"\nFinances: " + std::wstring(player.finances) +
            L"\nDistance to Club: " + std::wstring(player.distanceToClubKm) +
            L"\nHeight / Weight: " + std::wstring(player.height) + L" / " + std::wstring(player.weightKg) +
            L"\nPotential Height: " + std::wstring(player.potentialHeight);

        std::wstring xfactors =
            L"Mentality X-Factor: " + std::wstring(player.mentalityXFactor) +
            L"\nPhysical X-Factor: " + std::wstring(player.physicalXFactor) +
            L"\nWeaknesses: " + std::wstring(player.weaknesses);

        IdentityText().Text(hstring(identity));
        BackgroundText().Text(hstring(background));
        XFactorText().Text(hstring(xfactors));
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
        dialog.Title(box_value(L"Career Hub Coming Next"));
        dialog.Content(box_value(L"Next step: wire this confirmed player into CareerHubPage and start the weekly loop."));
        dialog.CloseButtonText(L"OK");
        dialog.XamlRoot(this->XamlRoot());
        dialog.ShowAsync();
    }
}

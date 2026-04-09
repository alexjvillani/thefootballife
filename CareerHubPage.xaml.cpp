#include "pch.h"
#include "CareerHubPage.xaml.h"
#if __has_include("CareerHubPage.g.cpp")
#include "CareerHubPage.g.cpp"
#endif

#include "GameState.h"
#include <winrt/Windows.Foundation.h>
#include <winrt/Microsoft.UI.Xaml.Media.Imaging.h>

#include <winrt/Windows.Storage.h>
#include <winrt/Windows.UI.Xaml.Interop.h>
#include <fstream>

using namespace winrt;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Media::Imaging;
using namespace Windows::Foundation;
using namespace Windows::Storage;

namespace winrt::thefootballife::implementation
{
    CareerHubPage::CareerHubPage()
    {
        InitializeComponent();
        LoadPlayerData();
        UpdateWeekDisplay();
    }

    hstring CareerHubPage::PageTitle()
    {
        return m_pageTitle;
    }

    void CareerHubPage::PageTitle(hstring const& value)
    {
        m_pageTitle = value;
    }

    hstring CareerHubPage::FormatHeightFeet(int totalCm)
    {
        double totalInches = static_cast<double>(totalCm) / 2.54;
        int roundedInches = static_cast<int>(totalInches + 0.5);
        int feet = roundedInches / 12;
        int inches = roundedInches % 12;

        return to_hstring(feet) + L"'" + to_hstring(inches) + L"\"";
    }

    void CareerHubPage::LoadPlayerData()
    {
        auto const& player = GameState::CurrentPlayer;

        if (!player.profileImagePath.empty())
        {
            BitmapImage bitmap;
            bitmap.UriSource(Uri(player.profileImagePath));
            ProfileImage().Source(bitmap);
        }

        PlayerNameText().Text(hstring(player.firstName + L" " + player.lastName));
        PlayerInfoText().Text(
            hstring(player.position + L" | " + player.foot + L" Foot | #" + player.number)
        );

        HeightText().Text(L"Height: " + FormatHeightFeet(player.heightCm));

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
    }

    void CareerHubPage::UpdateWeekDisplay()
    {
        WeekText().Text(L"Week " + to_hstring(m_currentWeek));
        LastChoiceText().Text(m_lastChoice);

        if (m_currentWeek <= 4)
        {
            StatusText().Text(L"Status: Local League Prospect");
            SeasonText().Text(L"Season Phase: School Season");
            WeeklyOutlookText().Text(L"A fresh week ahead. Focus on balancing development, performance, and life outside footy.");
            DevelopmentText().Text(L"Training form is steady. Recruiters have not yet locked onto your progress.");
        }
        else
        {
            StatusText().Text(L"Status: Emerging Prospect");
            SeasonText().Text(L"Season Phase: Mid-Season Push");
            WeeklyOutlookText().Text(L"Momentum is building. Your weekly choices are starting to shape how coaches and recruiters see you.");
            DevelopmentText().Text(L"Your development path is becoming clearer as consistency starts to matter more.");
        }
    }

    void CareerHubPage::TrainButton_Click(IInspectable const&, RoutedEventArgs const&)
    {
        m_lastChoice = L"Training selected. You focused on improving your game this week.";
        BottomHintText().Text(L"Training can improve long-term growth and sharpen performance.");
        UpdateWeekDisplay();
    }

    void CareerHubPage::PlayMatchButton_Click(IInspectable const&, RoutedEventArgs const&)
    {
        m_lastChoice = L"Play Match selected. This week will revolve around match performance.";
        BottomHintText().Text(L"Strong performances can lift confidence, selection chances, and recruiter interest.");
        UpdateWeekDisplay();
    }

    void CareerHubPage::RestButton_Click(IInspectable const&, RoutedEventArgs const&)
    {
        m_lastChoice = L"Rest / Social selected. You chose recovery and life balance this week.";
        BottomHintText().Text(L"Rest can help fatigue and mindset, but too much can slow development.");
        UpdateWeekDisplay();
    }

    void CareerHubPage::StudyButton_Click(IInspectable const&, RoutedEventArgs const&)
    {
        m_lastChoice = L"Study selected. You put time into school and off-field stability.";
        BottomHintText().Text(L"Balancing study can affect stress, discipline, and future pathways.");
        UpdateWeekDisplay();
    }

    void CareerHubPage::AdvanceWeekButton_Click(IInspectable const&, RoutedEventArgs const&)
    {
        m_currentWeek++;
        BottomHintText().Text(L"The week has advanced. Keep shaping your career with each decision.");
        UpdateWeekDisplay();
    }

    void CareerHubPage::SaveGameButton_Click(IInspectable const&, RoutedEventArgs const&)
    {
        auto const& player = GameState::CurrentPlayer;

        std::wstring savePath = ApplicationData::Current().LocalFolder().Path().c_str();
        savePath += L"\\career_save.txt";

        std::wofstream file(savePath);

        if (file.is_open())
        {
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
            file << L"CurrentWeek=" << m_currentWeek << L"\n";
            file << L"LastChoice=" << m_lastChoice.c_str() << L"\n";

            file.close();

            ContentDialog dialog;
            dialog.Title(box_value(L"Game Saved"));
            dialog.Content(box_value(L"Your career has been saved successfully."));
            dialog.CloseButtonText(L"OK");
            dialog.XamlRoot(this->XamlRoot());
            dialog.ShowAsync();
        }
        else
        {
            ContentDialog dialog;
            dialog.Title(box_value(L"Save Failed"));
            dialog.Content(box_value(L"Could not write the save file."));
            dialog.CloseButtonText(L"OK");
            dialog.XamlRoot(this->XamlRoot());
            dialog.ShowAsync();
        }
    }

    void CareerHubPage::ExitToMyCareerButton_Click(IInspectable const&, RoutedEventArgs const&)
    {
        Frame().Navigate(
            winrt::Windows::UI::Xaml::Interop::TypeName{
                L"thefootballife.MyCareerPage",
                winrt::Windows::UI::Xaml::Interop::TypeKind::Custom
            }
        );
    }
}
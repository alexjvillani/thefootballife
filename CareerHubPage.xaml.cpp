#include "pch.h"
#include "CareerHubPage.xaml.h"
#if __has_include("CareerHubPage.g.cpp")
#include "CareerHubPage.g.cpp"
#endif

#include "GameState.h"
#include "SaveGameService.h"
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
    CareerHubPage::CareerHubPage()
    {
        InitializeComponent();
        m_currentWeek = GameState::CurrentWeek;
        m_lastChoice = hstring(GameState::LastChoice);
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
        GameState::LastChoice = m_lastChoice.c_str();
        BottomHintText().Text(L"Training can improve long-term growth and sharpen performance.");
        UpdateWeekDisplay();
    }

    void CareerHubPage::PlayMatchButton_Click(IInspectable const&, RoutedEventArgs const&)
    {
        m_lastChoice = L"Play Match selected. This week will revolve around match performance.";
        GameState::LastChoice = m_lastChoice.c_str();
        BottomHintText().Text(L"Strong performances can lift confidence, selection chances, and recruiter interest.");
        UpdateWeekDisplay();
    }

    void CareerHubPage::RestButton_Click(IInspectable const&, RoutedEventArgs const&)
    {
        m_lastChoice = L"Rest / Social selected. You chose recovery and life balance this week.";
        GameState::LastChoice = m_lastChoice.c_str();
        BottomHintText().Text(L"Rest can help fatigue and mindset, but too much can slow development.");
        UpdateWeekDisplay();
    }

    void CareerHubPage::StudyButton_Click(IInspectable const&, RoutedEventArgs const&)
    {
        m_lastChoice = L"Study selected. You put time into school and off-field stability.";
        GameState::LastChoice = m_lastChoice.c_str();
        BottomHintText().Text(L"Balancing study can affect stress, discipline, and future pathways.");
        UpdateWeekDisplay();
    }

    void CareerHubPage::AdvanceWeekButton_Click(IInspectable const&, RoutedEventArgs const&)
    {
        m_currentWeek++;
        GameState::CurrentWeek = m_currentWeek;
        BottomHintText().Text(L"The week has advanced. Keep shaping your career with each decision.");
        UpdateWeekDisplay();
    }

    void CareerHubPage::SaveGameButton_Click(IInspectable const&, RoutedEventArgs const&)
    {
        ComboBox slotComboBox;
        for (int slot = 1; slot <= SaveGameService::MaxSaveSlots; ++slot)
        {
            ComboBoxItem item;
            std::wstring label = L"Slot " + std::to_wstring(slot);
            if (SaveGameService::SlotExists(slot))
            {
                label += L" (Overwrite)";
            }
            else
            {
                label += L" (Empty)";
            }

            item.Content(box_value(hstring(label)));
            slotComboBox.Items().Append(item);
        }

        int recommendedSlot = SaveGameService::FindFirstAvailableSlot();
        slotComboBox.SelectedIndex(recommendedSlot - 1);

        ContentDialog slotDialog;
        slotDialog.Title(box_value(L"Choose Save Slot"));
        slotDialog.Content(slotComboBox);
        slotDialog.PrimaryButtonText(L"Save");
        slotDialog.CloseButtonText(L"Cancel");
        slotDialog.XamlRoot(this->XamlRoot());

        auto weakThis = get_weak();
        slotDialog.ShowAsync().Completed(
            [weakThis, slotComboBox](auto const& operation, auto const&)
            {
                if (auto self = weakThis.get())
                {
                    if (operation.GetResults() != ContentDialogResult::Primary)
                    {
                        return;
                    }

                    int slot = static_cast<int>(slotComboBox.SelectedIndex()) + 1;
                    bool saved = SaveGameService::SaveToSlot(
                        slot,
                        GameState::CurrentPlayer,
                        self->m_currentWeek,
                        self->m_lastChoice.c_str()
                    );

                    ContentDialog resultDialog;
                    resultDialog.XamlRoot(self->XamlRoot());
                    resultDialog.CloseButtonText(L"OK");

                    if (saved)
                    {
                        resultDialog.Title(box_value(L"Game Saved"));
                        resultDialog.Content(
                            box_value(L"Career saved to slot " + to_hstring(slot) + L".")
                        );
                    }
                    else
                    {
                        resultDialog.Title(box_value(L"Save Failed"));
                        resultDialog.Content(box_value(L"Could not write the selected save slot."));
                    }

                    resultDialog.ShowAsync();
                }
            }
        );
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

    void CareerHubPage::ExitToMainMenuButton_Click(IInspectable const&, RoutedEventArgs const&)
    {
        Frame().Navigate(
            winrt::Windows::UI::Xaml::Interop::TypeName{
                L"thefootballife.MainMenuPage",
                winrt::Windows::UI::Xaml::Interop::TypeKind::Custom
            }
        );
    }
}

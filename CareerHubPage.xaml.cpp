#include "pch.h"
#include "CareerHubPage.xaml.h"
#if __has_include("CareerHubPage.g.cpp")
#include "CareerHubPage.g.cpp"
#endif

#include "GameState.h"
#include "SaveGameService.h"
#include <algorithm>
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
        UpdateBlockUI();
        UpdateStateUI();
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

    int CareerHubPage::BlocksUsed() const
    {
        return m_trainingBlocks + m_schoolBlocks + m_workBlocks + m_socialBlocks + m_recoveryBlocks;
    }

    void CareerHubPage::UpdateBlockUI()
    {
        TrainingBlocksText().Text(to_hstring(m_trainingBlocks));
        SchoolBlocksText().Text(to_hstring(m_schoolBlocks));
        WorkBlocksText().Text(to_hstring(m_workBlocks));
        SocialBlocksText().Text(to_hstring(m_socialBlocks));
        RecoveryBlocksText().Text(to_hstring(m_recoveryBlocks));

        int used = BlocksUsed();
        int remaining = kTotalBlocks - used;
        BlockSummaryText().Text(to_hstring(used) + L"/" + to_hstring(kTotalBlocks) + L" blocks allocated");

        if (remaining == 0)
        {
            BlockWarningText().Text(L"Allocation is valid. You can advance the week.");
        }
        else if (remaining > 0)
        {
            BlockWarningText().Text(L"Unassigned blocks: " + to_hstring(remaining) + L". Assign all blocks before advancing.");
        }
        else
        {
            BlockWarningText().Text(L"Over allocated by " + to_hstring(-remaining) + L". Remove blocks to continue.");
        }
    }

    void CareerHubPage::UpdateStateUI()
    {
        PhysicalStateText().Text(
            L"Fatigue: " + to_hstring(m_fatigue) +
            L" | Injury Risk: " + to_hstring(m_injuryRisk) +
            L" | Recovery Quality: " + to_hstring(m_recoveryQuality)
        );

        MentalStateText().Text(
            L"Confidence: " + to_hstring(m_confidence) +
            L" | Stress: " + to_hstring(m_stress) +
            L" | Motivation: " + to_hstring(m_motivation)
        );

        LifeStateText().Text(
            L"Discipline: " + to_hstring(m_discipline) +
            L" | Finances: " + to_hstring(m_finances) +
            L" | Relationships: " + to_hstring(m_relationships)
        );
    }

    void CareerHubPage::AdjustBlockByTag(hstring const& tag, int delta)
    {
        int* target = nullptr;

        if (tag == L"Training")
            target = &m_trainingBlocks;
        else if (tag == L"School")
            target = &m_schoolBlocks;
        else if (tag == L"Work")
            target = &m_workBlocks;
        else if (tag == L"Social")
            target = &m_socialBlocks;
        else if (tag == L"Recovery")
            target = &m_recoveryBlocks;

        if (target == nullptr)
            return;

        int proposed = *target + delta;
        if (proposed < 0)
        {
            BottomHintText().Text(L"Blocks cannot go below zero.");
            return;
        }

        int projectedTotal = BlocksUsed() + delta;
        if (delta > 0 && projectedTotal > kTotalBlocks)
        {
            BottomHintText().Text(L"You only have 14 total blocks each week.");
            return;
        }

        *target = proposed;
        BottomHintText().Text(L"Weekly schedule updated.");
        UpdateBlockUI();
    }

    void CareerHubPage::ApplyWeekSimulation()
    {
        m_fatigue = std::clamp(m_fatigue + (m_trainingBlocks * 4) + (m_workBlocks * 3) - (m_recoveryBlocks * 8), 0, 100);
        m_injuryRisk = std::clamp(m_injuryRisk + (m_fatigue / 12) + (m_trainingBlocks * 2) - (m_recoveryBlocks * 5), 0, 100);
        m_recoveryQuality = std::clamp(m_recoveryQuality + (m_recoveryBlocks * 7) - (m_workBlocks * 2), 0, 100);

        m_confidence = std::clamp(m_confidence + (m_trainingBlocks * 2) + (m_socialBlocks) - (m_stress / 18), 0, 100);
        m_stress = std::clamp(m_stress + (m_schoolBlocks * 2) + (m_workBlocks * 3) - (m_recoveryBlocks * 4), 0, 100);
        m_motivation = std::clamp(m_motivation + (m_trainingBlocks) + (m_socialBlocks) - (m_fatigue / 20), 0, 100);

        m_discipline = std::clamp(m_discipline + (m_schoolBlocks * 2) + (m_trainingBlocks) - (m_socialBlocks * 2), 0, 100);
        m_finances = std::clamp(m_finances + (m_workBlocks * 6) - (m_recoveryBlocks), 0, 100);
        m_relationships = std::clamp(m_relationships + (m_socialBlocks * 4) - (m_workBlocks), 0, 100);

        std::wstring consequence;
        if (m_recoveryBlocks == 0)
        {
            consequence += L"Lack of sleep lowered performance readiness and increased injury risk. ";
        }

        if (m_trainingBlocks >= 6)
        {
            consequence += L"Extra training boosted stats but pushed up fatigue. ";
        }

        if (m_workBlocks >= 4)
        {
            consequence += L"Heavy work schedule improved finances while reducing recovery quality. ";
        }

        if (m_socialBlocks >= 4)
        {
            consequence += L"Social time improved morale and relationships, but discipline dipped. ";
            m_discipline = std::clamp(m_discipline - 4, 0, 100);
        }

        if (consequence.empty())
        {
            consequence = L"Balanced week. No major penalties triggered.";
        }

        ConsequenceText().Text(hstring(consequence));

        m_lastChoice = L"Week simulated with blocks T:" + to_hstring(m_trainingBlocks) +
            L" S:" + to_hstring(m_schoolBlocks) +
            L" W:" + to_hstring(m_workBlocks) +
            L" So:" + to_hstring(m_socialBlocks) +
            L" R:" + to_hstring(m_recoveryBlocks);

        GameState::LastChoice = m_lastChoice.c_str();
        UpdateStateUI();
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
            WeeklyOutlookText().Text(L"Momentum is building. Your weekly choices are shaping both your football ceiling and off-field life.");
            DevelopmentText().Text(L"Coaches now track your consistency, discipline, and resilience week-to-week.");
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

    void CareerHubPage::IncrementBlockButton_Click(IInspectable const& sender, RoutedEventArgs const&)
    {
        auto button = sender.try_as<Button>();
        if (!button)
            return;

        auto tag = unbox_value_or<hstring>(button.Tag(), L"");
        AdjustBlockByTag(tag, 1);
    }

    void CareerHubPage::DecrementBlockButton_Click(IInspectable const& sender, RoutedEventArgs const&)
    {
        auto button = sender.try_as<Button>();
        if (!button)
            return;

        auto tag = unbox_value_or<hstring>(button.Tag(), L"");
        AdjustBlockByTag(tag, -1);
    }

    void CareerHubPage::AdvanceWeekButton_Click(IInspectable const&, RoutedEventArgs const&)
    {
        if (BlocksUsed() != kTotalBlocks)
        {
            BottomHintText().Text(L"You must allocate exactly 14 blocks before advancing.");
            return;
        }

        ApplyWeekSimulation();

        m_currentWeek++;
        GameState::CurrentWeek = m_currentWeek;
        BottomHintText().Text(L"Week advanced. Simulation outcomes applied to your player state.");
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

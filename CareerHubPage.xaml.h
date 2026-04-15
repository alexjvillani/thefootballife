#pragma once
#include "CareerHubPage.g.h"

namespace winrt::thefootballife::implementation
{
    struct CareerHubPage : CareerHubPageT<CareerHubPage>
    {
        CareerHubPage();

        winrt::hstring PageTitle();
        void PageTitle(winrt::hstring const& value);

        void TrainButton_Click(
            winrt::Windows::Foundation::IInspectable const& sender,
            winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);

        void PlayMatchButton_Click(
            winrt::Windows::Foundation::IInspectable const& sender,
            winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);

        void RestButton_Click(
            winrt::Windows::Foundation::IInspectable const& sender,
            winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);

        void StudyButton_Click(
            winrt::Windows::Foundation::IInspectable const& sender,
            winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);

        void IncrementBlockButton_Click(
            winrt::Windows::Foundation::IInspectable const& sender,
            winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);

        void DecrementBlockButton_Click(
            winrt::Windows::Foundation::IInspectable const& sender,
            winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);

        void SaveGameButton_Click(
            winrt::Windows::Foundation::IInspectable const& sender,
            winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);

        void ExitToMyCareerButton_Click(
            winrt::Windows::Foundation::IInspectable const& sender,
            winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);

        void AdvanceWeekButton_Click(
            winrt::Windows::Foundation::IInspectable const& sender,
            winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);

        void ExitToMainMenuButton_Click(
            winrt::Windows::Foundation::IInspectable const& sender,
            winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);

    private:
        void LoadPlayerData();
        void UpdateWeekDisplay();
        void UpdateBlockUI();
        void UpdateStateUI();
        void ApplyWeekSimulation();
        void AdjustBlockByTag(winrt::hstring const& tag, int delta);
        int BlocksUsed() const;
        winrt::hstring FormatHeightFeet(int totalCm);

    private:
        int m_currentWeek{ 1 };
        winrt::hstring m_lastChoice{ L"No action chosen yet." };
        winrt::hstring m_pageTitle{ L"Career Hub" };

        static constexpr int kTotalBlocks{ 14 };
        int m_trainingBlocks{ 4 };
        int m_schoolBlocks{ 5 };
        int m_workBlocks{ 2 };
        int m_socialBlocks{ 2 };
        int m_recoveryBlocks{ 1 };

        int m_fatigue{ 30 };
        int m_injuryRisk{ 20 };
        int m_recoveryQuality{ 55 };

        int m_confidence{ 55 };
        int m_stress{ 35 };
        int m_motivation{ 60 };

        int m_discipline{ 60 };
        int m_finances{ 35 };
        int m_relationships{ 50 };
    };
}

namespace winrt::thefootballife::factory_implementation
{
    struct CareerHubPage : CareerHubPageT<CareerHubPage, implementation::CareerHubPage>
    {
    };
}

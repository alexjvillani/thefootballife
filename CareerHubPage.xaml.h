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
        winrt::hstring FormatHeightFeet(int totalCm);

    private:
        int m_currentWeek{ 1 };
        winrt::hstring m_lastChoice{ L"No action chosen yet." };
        winrt::hstring m_pageTitle{ L"Career Hub" };
    };
}

namespace winrt::thefootballife::factory_implementation
{
    struct CareerHubPage : CareerHubPageT<CareerHubPage, implementation::CareerHubPage>
    {
    };
}
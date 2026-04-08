#pragma once
#include "PlayerCreationPage.g.h"

namespace winrt::thefootballife::implementation
{
    struct PlayerCreationPage : PlayerCreationPageT<PlayerCreationPage>
    {
        PlayerCreationPage();

        void BackButton_Click(
            winrt::Windows::Foundation::IInspectable const& sender,
            winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);

        void ContinueButton_Click(
            winrt::Windows::Foundation::IInspectable const& sender,
            winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);

        void GeneratePreview_Click(
            winrt::Windows::Foundation::IInspectable const& sender,
            winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);

        void ManualPhysicalCheckBox_Changed(
            winrt::Windows::Foundation::IInspectable const& sender,
            winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);

        void NameField_Changed(
            winrt::Windows::Foundation::IInspectable const& sender,
            winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);

        void PlayerField_Changed(
            winrt::Windows::Foundation::IInspectable const& sender,
            winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);

        void PlayerField_Changed(
            winrt::Windows::Foundation::IInspectable const& sender,
            winrt::Microsoft::UI::Xaml::Controls::SelectionChangedEventArgs const& e);

    private:
        winrt::hstring GetComboBoxValue(winrt::Microsoft::UI::Xaml::Controls::ComboBox const& comboBox);
        winrt::hstring GetFullName();
        winrt::hstring FormatHeightFeet(int totalCm);
        int ParseFeetAndInches(winrt::hstring const& text);
        int ParseWeight(winrt::hstring const& text);
        void SetRandomProfileImage();
        void UpdateGeneratedProfile();

    private:
        bool m_isPageReady{ false };

        int m_generatedHeightCm{ 0 };
        int m_generatedWeightKg{ 0 };
        int m_potentialHeightCm{ 0 };
        int m_distanceToClubKm{ 0 };

        winrt::hstring m_familySituation{ L"" };
        winrt::hstring m_finances{ L"" };
        winrt::hstring m_schoolQuality{ L"" };
    };
}

namespace winrt::thefootballife::factory_implementation
{
    struct PlayerCreationPage : PlayerCreationPageT<PlayerCreationPage, implementation::PlayerCreationPage>
    {
    };
}

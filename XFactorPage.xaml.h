#pragma once
#include "XFactorPage.g.h"

namespace winrt::thefootballife::implementation
{
    struct XFactorPage : XFactorPageT<XFactorPage>
    {
        XFactorPage();

        winrt::hstring PageTitle();
        void PageTitle(winrt::hstring const& value);

        void BackButton_Click(
            winrt::Windows::Foundation::IInspectable const& sender,
            winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);

        void ContinueButton_Click(
            winrt::Windows::Foundation::IInspectable const& sender,
            winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);

        void MentalityComboBox_SelectionChanged(
            winrt::Windows::Foundation::IInspectable const& sender,
            winrt::Microsoft::UI::Xaml::Controls::SelectionChangedEventArgs const& e);

        void PhysicalComboBox_SelectionChanged(
            winrt::Windows::Foundation::IInspectable const& sender,
            winrt::Microsoft::UI::Xaml::Controls::SelectionChangedEventArgs const& e);

        void WeaknessCheckBox_Changed(
            winrt::Windows::Foundation::IInspectable const& sender,
            winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);

    private:
        winrt::hstring GetSelectedComboValue(winrt::Microsoft::UI::Xaml::Controls::ComboBox const& comboBox);
        winrt::hstring BuildWeaknessSummary();
        void UpdateTraitSummary();

    private:
        bool m_isPageReady{ false };
        winrt::hstring m_pageTitle{ L"X-Factors" };
    };
}

namespace winrt::thefootballife::factory_implementation
{
    struct XFactorPage : XFactorPageT<XFactorPage, implementation::XFactorPage>
    {
    };
}

#pragma once
#include "ConfirmPlayerPage.g.h"

namespace winrt::thefootballife::implementation
{
    struct ConfirmPlayerPage : ConfirmPlayerPageT<ConfirmPlayerPage>
    {
        ConfirmPlayerPage();

        void BackButton_Click(
            winrt::Windows::Foundation::IInspectable const& sender,
            winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);

        void StartCareerButton_Click(
            winrt::Windows::Foundation::IInspectable const& sender,
            winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);

    private:
        void LoadPlayerSummary();
    };
}

namespace winrt::thefootballife::factory_implementation
{
    struct ConfirmPlayerPage : ConfirmPlayerPageT<ConfirmPlayerPage, implementation::ConfirmPlayerPage>
    {
    };
}

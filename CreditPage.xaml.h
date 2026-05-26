#pragma once

#include "CreditPage.g.h"

namespace winrt::thefootballife::implementation
{
    struct CreditPage : CreditPageT<CreditPage>
    {
        CreditPage();

        void BackButton_Click(
            winrt::Windows::Foundation::IInspectable const& sender,
            winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);

        bool operator==(const CreditPage& other) const
        {
            return false;
        }
    };
}

namespace winrt::thefootballife::factory_implementation
{
    struct CreditPage : CreditPageT<CreditPage, implementation::CreditPage>
    {
        CreditPage() = default;
    };
}
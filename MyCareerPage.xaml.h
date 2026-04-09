#pragma once
#include "MyCareerPage.g.h"

namespace winrt::thefootballife::implementation
{
    struct MyCareerPage : MyCareerPageT<MyCareerPage>
    {
        MyCareerPage();

        winrt::hstring PageTitle();
        void PageTitle(winrt::hstring const& value);

        void BackToCareerHubButton_Click(
            winrt::Windows::Foundation::IInspectable const& sender,
            winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);

    private:
        void LoadCareerData();

        winrt::hstring m_pageTitle{ L"My Career" };
    };
}

namespace winrt::thefootballife::factory_implementation
{
    struct MyCareerPage : MyCareerPageT<MyCareerPage, implementation::MyCareerPage>
    {
    };
}
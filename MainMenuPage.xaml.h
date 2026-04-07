#pragma once
#include "MainMenuPage.g.h"

namespace winrt::thefootballife::implementation
{
    struct MainMenuPage : MainMenuPageT<MainMenuPage>
    {
        MainMenuPage();

        void NewGame_Click(winrt::Windows::Foundation::IInspectable const& sender,
            winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        void LoadGame_Click(winrt::Windows::Foundation::IInspectable const& sender,
            winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        void Settings_Click(winrt::Windows::Foundation::IInspectable const& sender,
            winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        void Credits_Click(winrt::Windows::Foundation::IInspectable const& sender,
            winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        void Exit_Click(winrt::Windows::Foundation::IInspectable const& sender,
            winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
    };
}

namespace winrt::thefootballife::factory_implementation
{
    struct MainMenuPage : MainMenuPageT<MainMenuPage, implementation::MainMenuPage>
    {
    };
}
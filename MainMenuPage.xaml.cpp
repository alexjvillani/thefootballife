#include "pch.h"
#include "MainMenuPage.xaml.h"
#if __has_include("MainMenuPage.g.cpp")
#include "MainMenuPage.g.cpp"
#endif

#include "PlayerCreationPage.xaml.h"
#include <winrt/Windows.UI.Xaml.Interop.h>

using namespace winrt;
using namespace Windows::Foundation;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;

namespace winrt::thefootballife::implementation
{
    MainMenuPage::MainMenuPage()
    {
        InitializeComponent();
    }

    void MainMenuPage::NewGame_Click(IInspectable const&, RoutedEventArgs const&)
    {
        Frame().Navigate(
            winrt::Windows::UI::Xaml::Interop::TypeName{
                L"thefootballife.PlayerCreationPage",
                winrt::Windows::UI::Xaml::Interop::TypeKind::Custom
            }
        );
    }

    void MainMenuPage::LoadGame_Click(IInspectable const&, RoutedEventArgs const&)
    {
        ContentDialog dialog;
        dialog.Title(box_value(L"Load Game"));
        dialog.Content(box_value(L"Save/load system coming soon."));
        dialog.CloseButtonText(L"OK");
        dialog.XamlRoot(this->XamlRoot());
        dialog.ShowAsync();
    }

    void MainMenuPage::Settings_Click(IInspectable const&, RoutedEventArgs const&)
    {
        ContentDialog dialog;
        dialog.Title(box_value(L"Settings"));
        dialog.Content(box_value(L"Settings menu coming soon."));
        dialog.CloseButtonText(L"OK");
        dialog.XamlRoot(this->XamlRoot());
        dialog.ShowAsync();
    }

    void MainMenuPage::Credits_Click(IInspectable const&, RoutedEventArgs const&)
    {
        ContentDialog dialog;
        dialog.Title(box_value(L"Credits"));
        dialog.Content(box_value(L"The Football Life\nCreated by Alex Villani"));
        dialog.CloseButtonText(L"OK");
        dialog.XamlRoot(this->XamlRoot());
        dialog.ShowAsync();
    }

    void MainMenuPage::Exit_Click(IInspectable const&, RoutedEventArgs const&)
    {
        Application::Current().Exit();
    }
}
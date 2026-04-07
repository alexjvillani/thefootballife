#include "pch.h"
#include "MainWindow.xaml.h"
#include <winrt/Microsoft.UI.Xaml.Media.Animation.h>
#include <winrt/Windows.Foundation.h>
#if __has_include("MainWindow.g.cpp")
#include "MainWindow.g.cpp"
#endif

#include <chrono>

using namespace winrt;
using namespace Windows::Foundation;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Media::Animation;

namespace winrt::thefootballife::implementation
{
    MainWindow::MainWindow()
    {
        InitializeComponent();

        RootGrid().Opacity(0.0);

        using namespace Microsoft::UI::Xaml::Media::Animation;

        DoubleAnimation fadeAnim{};
        fadeAnim.From(0.0);
        fadeAnim.To(1.0);

        fadeAnim.Duration(Microsoft::UI::Xaml::DurationHelper::FromTimeSpan(
            std::chrono::milliseconds(500)
        ));

        Storyboard storyboard{};
        storyboard.Children().Append(fadeAnim);

        Storyboard::SetTarget(fadeAnim, RootGrid());
        Storyboard::SetTargetProperty(fadeAnim, L"Opacity");

        storyboard.Begin();
    }

    void MainWindow::NewGame_Click(IInspectable const&, RoutedEventArgs const&)
    {
        ContentDialog dialog;
        dialog.Title(box_value(L"New Game"));
        dialog.Content(box_value(L"Player creation will go here."));
        dialog.CloseButtonText(L"OK");
        dialog.XamlRoot(this->Content().XamlRoot());
        dialog.ShowAsync();
    }

    void MainWindow::LoadGame_Click(IInspectable const&, RoutedEventArgs const&)
    {
        ContentDialog dialog;
        dialog.Title(box_value(L"Load Game"));
        dialog.Content(box_value(L"Save/load system coming soon."));
        dialog.CloseButtonText(L"OK");
        dialog.XamlRoot(this->Content().XamlRoot());
        dialog.ShowAsync();
    }

    void MainWindow::Settings_Click(IInspectable const&, RoutedEventArgs const&)
    {
        ContentDialog dialog;
        dialog.Title(box_value(L"Settings"));
        dialog.Content(box_value(L"Settings menu coming soon."));
        dialog.CloseButtonText(L"OK");
        dialog.XamlRoot(this->Content().XamlRoot());
        dialog.ShowAsync();
    }

    void MainWindow::Credits_Click(IInspectable const&, RoutedEventArgs const&)
    {
        ContentDialog dialog;
        dialog.Title(box_value(L"Credits"));
        dialog.Content(box_value(L"The Football Life\nCreated by Alex Villani"));
        dialog.CloseButtonText(L"OK");
        dialog.XamlRoot(this->Content().XamlRoot());
        dialog.ShowAsync();
    }

    void MainWindow::Exit_Click(IInspectable const&, RoutedEventArgs const&)
    {
        this->Close();
    }
}
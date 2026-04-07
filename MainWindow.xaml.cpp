#include "pch.h"
#include "MainWindow.xaml.h"
#include "MainMenuPage.xaml.h"
#if __has_include("MainWindow.g.cpp")
#include "MainWindow.g.cpp"
#endif

#include <winrt/Windows.UI.Xaml.Interop.h>

using namespace winrt;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;

namespace winrt::thefootballife::implementation
{
    MainWindow::MainWindow()
    {
        InitializeComponent();

        RootFrame().Navigate(
            winrt::Windows::UI::Xaml::Interop::TypeName{
                L"thefootballife.MainMenuPage",
                winrt::Windows::UI::Xaml::Interop::TypeKind::Custom
            }
        );
    }
}
#include "pch.h"
#include "MainWindow.xaml.h"
#include "MainMenuPage.xaml.h"
#if __has_include("MainWindow.g.cpp")
#include "MainWindow.g.cpp"
#endif

#include <winrt/Windows.UI.Xaml.Interop.h>
#include <winrt/Microsoft.UI.Windowing.h>

using namespace winrt;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;

namespace winrt::thefootballife::implementation
{
    MainWindow::MainWindow()
    {
        InitializeComponent();

        auto appWindow = this->AppWindow();
        if (auto presenter = appWindow.Presenter().try_as<Microsoft::UI::Windowing::OverlappedPresenter>())
        {
            presenter.Maximize();
        }

        RootFrame().Navigate(
            winrt::Windows::UI::Xaml::Interop::TypeName{
                L"thefootballife.MainMenuPage",
                winrt::Windows::UI::Xaml::Interop::TypeKind::Custom
            }
        );
    }
}
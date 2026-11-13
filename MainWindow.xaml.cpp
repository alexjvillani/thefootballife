#include "pch.h"
#include "MainWindow.xaml.h"
#include "MainMenuPage.xaml.h"
#if __has_include("MainWindow.g.cpp")
#include "MainWindow.g.cpp"
#endif

#include "DisplaySettings.h"

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

		// Saved display options (window mode / size / UI scale). With no
		// settings file this is the original behaviour: start maximised.
		DisplaySettings::Load();
		DisplaySettings::Attach(this->AppWindow(), RootHost(), RootFrame());
		DisplaySettings::ApplyWindow();

		this->Closed([](auto&&, auto&&)
			{
				DisplaySettings::Detach();
			});

		RootFrame().Navigate(
			winrt::Windows::UI::Xaml::Interop::TypeName{
				L"thefootballife.MainMenuPage",
				winrt::Windows::UI::Xaml::Interop::TypeKind::Custom
			}
		);
	}

	void MainWindow::RootHost_SizeChanged(
		winrt::Windows::Foundation::IInspectable const&,
		winrt::Microsoft::UI::Xaml::SizeChangedEventArgs const&)
	{
		DisplaySettings::ApplyScale();
	}
}
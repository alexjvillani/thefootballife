#pragma once

#include "MainWindow.g.h"

namespace winrt::thefootballife::implementation
{
	struct MainWindow : MainWindowT<MainWindow>
	{
		MainWindow();

		void RootHost_SizeChanged(
			winrt::Windows::Foundation::IInspectable const& sender,
			winrt::Microsoft::UI::Xaml::SizeChangedEventArgs const& e);
	};
}

namespace winrt::thefootballife::factory_implementation
{
	struct MainWindow : MainWindowT<MainWindow, implementation::MainWindow>
	{
	};
}
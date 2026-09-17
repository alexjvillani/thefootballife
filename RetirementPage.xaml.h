#pragma once
#include "RetirementPage.g.h"

namespace winrt::thefootballife::implementation
{
	struct RetirementPage : RetirementPageT<RetirementPage>
	{
		RetirementPage();

		winrt::hstring PageTitle();
		void PageTitle(winrt::hstring const& value);

		void ReturnToMainMenuButton_Click(
			winrt::Windows::Foundation::IInspectable const& sender,
			winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);

	private:
		winrt::hstring m_pageTitle{ L"Retirement" };
	};
}

namespace winrt::thefootballife::factory_implementation
{
	struct RetirementPage : RetirementPageT<RetirementPage, implementation::RetirementPage>
	{
	};
}
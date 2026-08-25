#pragma once
#include "XFactorPage.g.h"
#include "XFactorService.h"
#include <vector>

namespace winrt::thefootballife::implementation
{
	struct XFactorPage : XFactorPageT<XFactorPage>
	{
		XFactorPage();

		winrt::hstring PageTitle();
		void PageTitle(winrt::hstring const& value);

		void BackButton_Click(
			winrt::Windows::Foundation::IInspectable const& sender,
			winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);

		void ContinueButton_Click(
			winrt::Windows::Foundation::IInspectable const& sender,
			winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);

		void WeaknessCheckBox_Changed(
			winrt::Windows::Foundation::IInspectable const& sender,
			winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);

	private:
		void PopulateMentalityCards();
		void PopulatePhysicalCards();
		void RestyleMentalityCards();
		void RestylePhysicalCards();
		void SelectMentality(int index);
		void SelectPhysical(int index);
		void UpdateTraitSummary();

	private:
		bool m_isPageReady{ false };
		winrt::hstring m_pageTitle{ L"X-Factors" };

		std::vector<XFactorService::XFactorOption> m_mentalityOptions;
		std::vector<XFactorService::XFactorOption> m_physicalOptions;
		int m_selectedMentalityIndex{ 0 };
		int m_selectedPhysicalIndex{ 0 };

		// Parallel to m_mentalityOptions/m_physicalOptions - kept so a
		// selection change can restyle every card (highlight the new pick,
		// un-highlight the rest) without re-querying the visual tree.
		std::vector<winrt::Microsoft::UI::Xaml::Controls::Border> m_mentalityCardBorders;
		std::vector<winrt::Microsoft::UI::Xaml::Controls::TextBlock> m_mentalityCardTitles;
		std::vector<winrt::Microsoft::UI::Xaml::Controls::Border> m_physicalCardBorders;
		std::vector<winrt::Microsoft::UI::Xaml::Controls::TextBlock> m_physicalCardTitles;
	};
}

namespace winrt::thefootballife::factory_implementation
{
	struct XFactorPage : XFactorPageT<XFactorPage, implementation::XFactorPage>
	{
	};
}
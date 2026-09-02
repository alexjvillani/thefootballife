#pragma once
#include "DraftNightPage.g.h"
#include <vector>
#include <string>

namespace winrt::thefootballife::implementation
{
	struct DraftNightPage : DraftNightPageT<DraftNightPage>
	{
		DraftNightPage();

		winrt::hstring PageTitle();
		void PageTitle(winrt::hstring const& value);

		void RevealButton_Click(
			winrt::Windows::Foundation::IInspectable const& sender,
			winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);

		void ContinueButton_Click(
			winrt::Windows::Foundation::IInspectable const& sender,
			winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);

	private:
		// Mirrors TeamAssignmentPage::TeamProfile - kept as a separate type
		// rather than shared, since the two pages don't otherwise share a
		// header and this struct is tiny.
		struct ClubProfile
		{
			std::wstring name;
			std::wstring suburb;
			std::wstring primaryColour;
			std::wstring secondaryColour;
			std::wstring homeGround;
			std::wstring league;
			int reputation{ 50 };
		};

		std::vector<ClubProfile> LoadTargetTierClubs() const;
		void FinalizeSeasonForNewClub();

	private:
		winrt::hstring m_pageTitle{ L"Draft Night" };
		std::vector<ClubProfile> m_targetClubs;
		ClubProfile m_revealedClub;
		bool m_hasRevealed{ false };
	};
}

namespace winrt::thefootballife::factory_implementation
{
	struct DraftNightPage : DraftNightPageT<DraftNightPage, implementation::DraftNightPage>
	{
	};
}
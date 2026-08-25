#include "pch.h"
#include "XFactorPage.xaml.h"
#include "Gamestate.h"
#if __has_include("XFactorPage.g.cpp")
#include "XFactorPage.g.cpp"
#include <winrt/Windows.UI.Xaml.Interop.h>
#endif

#include <algorithm>
#include <winrt/Windows.Foundation.h>
#include <winrt/Microsoft.UI.Xaml.Media.Imaging.h>
#include <winrt/Windows.UI.h>
#include <winrt/Windows.UI.Text.h>
#include <winrt/Microsoft.UI.Xaml.Input.h>

using namespace winrt;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Media::Imaging;
using namespace Microsoft::UI::Xaml::Input;

namespace
{
	// Fatigue, InjuryRisk and Stress are "lower is better" stats - everything
	// else is "higher is better". A pill's colour depends on this, not just
	// the sign of the delta, so a +3 InjuryRisk reads as a warning (red)
	// rather than a false positive (green).
	bool IsLowerBetter(std::wstring const& statKey)
	{
		return statKey == L"Fatigue" || statKey == L"InjuryRisk" || statKey == L"Stress";
	}

	std::wstring FormatStatLabel(std::wstring const& statKey)
	{
		if (statKey == L"InjuryRisk") return L"Injury Risk";
		if (statKey == L"RecoveryQuality") return L"Recovery";
		return statKey;
	}

	// StatDeltas is an unordered_map, so iterating it directly would render
	// a card's pills in a different order every page load. Sort
	// alphabetically for a stable, predictable layout.
	std::vector<std::pair<std::wstring, int>> SortedDeltas(std::unordered_map<std::wstring, int> const& deltas)
	{
		std::vector<std::pair<std::wstring, int>> out(deltas.begin(), deltas.end());
		std::sort(out.begin(), out.end(), [](auto const& a, auto const& b) { return a.first < b.first; });
		return out;
	}
}

namespace winrt::thefootballife::implementation
{
	XFactorPage::XFactorPage()
	{
		InitializeComponent();

		m_mentalityOptions = XFactorService::LoadOptions(L"Mentality");
		m_physicalOptions = XFactorService::LoadOptions(L"Physical");

		// Defensive fallback so the page never ends up with an empty card
		// list if xfactors.csv can't be found at runtime - same FindCsv
		// path-resolution risk events.csv has (check Assets\Data\ placement
		// and "Copy to Output Directory").
		if (m_mentalityOptions.empty())
		{
			XFactorService::XFactorOption fallback;
			fallback.Id = L"balanced_mentality";
			fallback.Title = L"Balanced";
			fallback.Description = L"A well-rounded head on his shoulders - no real edge, but no real gaps either.";
			m_mentalityOptions.push_back(fallback);
		}
		if (m_physicalOptions.empty())
		{
			XFactorService::XFactorOption fallback;
			fallback.Id = L"balanced_physical";
			fallback.Title = L"Balanced";
			fallback.Description = L"A solid, unremarkable athlete - no standout physical trait yet.";
			m_physicalOptions.push_back(fallback);
		}

		PopulateMentalityCards();
		PopulatePhysicalCards();

		m_isPageReady = true;

		if (MentalityDescriptionText())
		{
			MentalityDescriptionText().Text(hstring(m_mentalityOptions[m_selectedMentalityIndex].Description));
		}

		if (PhysicalDescriptionText())
		{
			PhysicalDescriptionText().Text(hstring(m_physicalOptions[m_selectedPhysicalIndex].Description));
		}

		UpdateTraitSummary();
	}

	void XFactorPage::PopulateMentalityCards()
	{
		if (!MentalityOptionsPanel())
		{
			return;
		}

		MentalityOptionsPanel().Children().Clear();
		m_mentalityCardBorders.clear();
		m_mentalityCardTitles.clear();

		for (int i = 0; i < static_cast<int>(m_mentalityOptions.size()); ++i)
		{
			auto const& opt = m_mentalityOptions[i];

			Border card;
			Microsoft::UI::Xaml::CornerRadius cardCr{};
			cardCr.TopLeft = cardCr.TopRight = cardCr.BottomRight = cardCr.BottomLeft = 8;
			card.CornerRadius(cardCr);
			card.Padding(Thickness{ 10, 8, 10, 8 });

			StackPanel content;
			content.Spacing(4);

			TextBlock title;
			title.Text(hstring(opt.Title));
			title.FontWeight(winrt::Windows::UI::Text::FontWeights::SemiBold());
			title.FontSize(14);
			content.Children().Append(title);

			auto deltas = SortedDeltas(opt.StatDeltas);
			if (!deltas.empty())
			{
				StackPanel pillRow;
				pillRow.Orientation(Orientation::Horizontal);
				pillRow.Spacing(6);

				for (auto const& kv : deltas)
				{
					bool isGood = (kv.second > 0) != IsLowerBetter(kv.first);
					auto bgColour = isGood
						? winrt::Windows::UI::ColorHelper::FromArgb(255, 31, 58, 42)
						: winrt::Windows::UI::ColorHelper::FromArgb(255, 58, 31, 31);
					auto textColour = isGood
						? winrt::Windows::UI::ColorHelper::FromArgb(255, 143, 224, 174)
						: winrt::Windows::UI::ColorHelper::FromArgb(255, 232, 154, 154);

					Border pill;
					pill.Background(SolidColorBrush(bgColour));
					Microsoft::UI::Xaml::CornerRadius pillCr{};
					pillCr.TopLeft = pillCr.TopRight = pillCr.BottomRight = pillCr.BottomLeft = 8;
					pill.CornerRadius(pillCr);
					pill.Padding(Thickness{ 7, 2, 7, 2 });

					TextBlock pillText;
					std::wstring sign = kv.second > 0 ? L"+" : L"";
					pillText.Text(hstring(FormatStatLabel(kv.first) + L" " + sign + std::to_wstring(kv.second)));
					pillText.FontSize(11);
					pillText.Foreground(SolidColorBrush(textColour));
					pill.Child(pillText);

					pillRow.Children().Append(pill);
				}

				content.Children().Append(pillRow);
			}

			card.Child(content);

			int index = i; // captured by value - each card remembers its own slot
			card.Tapped([this, index](IInspectable const&, winrt::Microsoft::UI::Xaml::Input::TappedRoutedEventArgs const&)
				{
					SelectMentality(index);
				});

			MentalityOptionsPanel().Children().Append(card);
			m_mentalityCardBorders.push_back(card);
			m_mentalityCardTitles.push_back(title);
		}

		RestyleMentalityCards();
	}

	void XFactorPage::PopulatePhysicalCards()
	{
		if (!PhysicalOptionsPanel())
		{
			return;
		}

		PhysicalOptionsPanel().Children().Clear();
		m_physicalCardBorders.clear();
		m_physicalCardTitles.clear();

		for (int i = 0; i < static_cast<int>(m_physicalOptions.size()); ++i)
		{
			auto const& opt = m_physicalOptions[i];

			Border card;
			Microsoft::UI::Xaml::CornerRadius cardCr{};
			cardCr.TopLeft = cardCr.TopRight = cardCr.BottomRight = cardCr.BottomLeft = 8;
			card.CornerRadius(cardCr);
			card.Padding(Thickness{ 10, 8, 10, 8 });

			StackPanel content;
			content.Spacing(4);

			TextBlock title;
			title.Text(hstring(opt.Title));
			title.FontWeight(winrt::Windows::UI::Text::FontWeights::SemiBold());
			title.FontSize(14);
			content.Children().Append(title);

			auto deltas = SortedDeltas(opt.StatDeltas);
			if (!deltas.empty())
			{
				StackPanel pillRow;
				pillRow.Orientation(Orientation::Horizontal);
				pillRow.Spacing(6);

				for (auto const& kv : deltas)
				{
					bool isGood = (kv.second > 0) != IsLowerBetter(kv.first);
					auto bgColour = isGood
						? winrt::Windows::UI::ColorHelper::FromArgb(255, 31, 58, 42)
						: winrt::Windows::UI::ColorHelper::FromArgb(255, 58, 31, 31);
					auto textColour = isGood
						? winrt::Windows::UI::ColorHelper::FromArgb(255, 143, 224, 174)
						: winrt::Windows::UI::ColorHelper::FromArgb(255, 232, 154, 154);

					Border pill;
					pill.Background(SolidColorBrush(bgColour));
					Microsoft::UI::Xaml::CornerRadius pillCr{};
					pillCr.TopLeft = pillCr.TopRight = pillCr.BottomRight = pillCr.BottomLeft = 8;
					pill.CornerRadius(pillCr);
					pill.Padding(Thickness{ 7, 2, 7, 2 });

					TextBlock pillText;
					std::wstring sign = kv.second > 0 ? L"+" : L"";
					pillText.Text(hstring(FormatStatLabel(kv.first) + L" " + sign + std::to_wstring(kv.second)));
					pillText.FontSize(11);
					pillText.Foreground(SolidColorBrush(textColour));
					pill.Child(pillText);

					pillRow.Children().Append(pill);
				}

				content.Children().Append(pillRow);
			}

			card.Child(content);

			int index = i;
			card.Tapped([this, index](IInspectable const&, winrt::Microsoft::UI::Xaml::Input::TappedRoutedEventArgs const&)
				{
					SelectPhysical(index);
				});

			PhysicalOptionsPanel().Children().Append(card);
			m_physicalCardBorders.push_back(card);
			m_physicalCardTitles.push_back(title);
		}

		RestylePhysicalCards();
	}

	void XFactorPage::RestyleMentalityCards()
	{
		for (int i = 0; i < static_cast<int>(m_mentalityCardBorders.size()); ++i)
		{
			bool selected = (i == m_selectedMentalityIndex);

			auto bg = selected
				? winrt::Windows::UI::ColorHelper::FromArgb(255, 46, 39, 23)
				: winrt::Windows::UI::ColorHelper::FromArgb(255, 30, 30, 30);
			m_mentalityCardBorders[i].Background(SolidColorBrush(bg));

			auto borderColour = selected
				? winrt::Windows::UI::ColorHelper::FromArgb(255, 212, 162, 76)
				: winrt::Windows::UI::ColorHelper::FromArgb(0, 0, 0, 0);
			m_mentalityCardBorders[i].BorderBrush(SolidColorBrush(borderColour));
			double thickness = selected ? 2.0 : 0.0;
			m_mentalityCardBorders[i].BorderThickness(Thickness{ thickness, thickness, thickness, thickness });

			auto titleColour = selected
				? winrt::Windows::UI::ColorHelper::FromArgb(255, 212, 162, 76)
				: winrt::Windows::UI::Colors::White();
			m_mentalityCardTitles[i].Foreground(SolidColorBrush(titleColour));
		}
	}

	void XFactorPage::RestylePhysicalCards()
	{
		for (int i = 0; i < static_cast<int>(m_physicalCardBorders.size()); ++i)
		{
			bool selected = (i == m_selectedPhysicalIndex);

			auto bg = selected
				? winrt::Windows::UI::ColorHelper::FromArgb(255, 46, 39, 23)
				: winrt::Windows::UI::ColorHelper::FromArgb(255, 30, 30, 30);
			m_physicalCardBorders[i].Background(SolidColorBrush(bg));

			auto borderColour = selected
				? winrt::Windows::UI::ColorHelper::FromArgb(255, 212, 162, 76)
				: winrt::Windows::UI::ColorHelper::FromArgb(0, 0, 0, 0);
			m_physicalCardBorders[i].BorderBrush(SolidColorBrush(borderColour));
			double thickness = selected ? 2.0 : 0.0;
			m_physicalCardBorders[i].BorderThickness(Thickness{ thickness, thickness, thickness, thickness });

			auto titleColour = selected
				? winrt::Windows::UI::ColorHelper::FromArgb(255, 212, 162, 76)
				: winrt::Windows::UI::Colors::White();
			m_physicalCardTitles[i].Foreground(SolidColorBrush(titleColour));
		}
	}

	void XFactorPage::SelectMentality(int index)
	{
		if (index < 0 || index >= static_cast<int>(m_mentalityOptions.size()))
		{
			return;
		}

		m_selectedMentalityIndex = index;
		RestyleMentalityCards();

		if (MentalityDescriptionText())
		{
			MentalityDescriptionText().Text(hstring(m_mentalityOptions[index].Description));
		}

		UpdateTraitSummary();
	}

	void XFactorPage::SelectPhysical(int index)
	{
		if (index < 0 || index >= static_cast<int>(m_physicalOptions.size()))
		{
			return;
		}

		m_selectedPhysicalIndex = index;
		RestylePhysicalCards();

		if (PhysicalDescriptionText())
		{
			PhysicalDescriptionText().Text(hstring(m_physicalOptions[index].Description));
		}

		UpdateTraitSummary();
	}

	hstring XFactorPage::PageTitle()
	{
		return m_pageTitle;
	}

	void XFactorPage::PageTitle(hstring const& value)
	{
		m_pageTitle = value;
	}

	void XFactorPage::WeaknessCheckBox_Changed(IInspectable const&, RoutedEventArgs const&)
	{
		if (!m_isPageReady)
		{
			return;
		}

		UpdateTraitSummary();
	}

	void XFactorPage::UpdateTraitSummary()
	{
		if (!TraitSummaryText())
		{
			return;
		}

		std::wstring mentality = (m_selectedMentalityIndex >= 0 && m_selectedMentalityIndex < static_cast<int>(m_mentalityOptions.size()))
			? m_mentalityOptions[m_selectedMentalityIndex].Title
			: L"Balanced";

		std::wstring physical = (m_selectedPhysicalIndex >= 0 && m_selectedPhysicalIndex < static_cast<int>(m_physicalOptions.size()))
			? m_physicalOptions[m_selectedPhysicalIndex].Title
			: L"Balanced";

		std::wstring weaknesses;
		if (WeaknessKick().IsChecked() && WeaknessKick().IsChecked().Value())
			weaknesses += L"Inconsistent Kicking, ";
		if (WeaknessFitness().IsChecked() && WeaknessFitness().IsChecked().Value())
			weaknesses += L"Low Endurance, ";
		if (WeaknessDecision().IsChecked() && WeaknessDecision().IsChecked().Value())
			weaknesses += L"Poor Decision Making, ";
		if (WeaknessPressure().IsChecked() && WeaknessPressure().IsChecked().Value())
			weaknesses += L"Struggles Under Pressure, ";

		std::wstring summary = L"Mentality: " + mentality + L"\nPhysical: " + physical;

		if (!weaknesses.empty())
		{
			weaknesses.erase(weaknesses.size() - 2);
			summary += L"\nWeaknesses: " + weaknesses;
		}
		else
		{
			summary += L"\nWeaknesses: None selected";
		}

		TraitSummaryText().Text(hstring(summary));
	}

	void XFactorPage::BackButton_Click(IInspectable const&, RoutedEventArgs const&)
	{
		if (Frame().CanGoBack())
		{
			Frame().GoBack();
		}
	}

	void XFactorPage::ContinueButton_Click(IInspectable const&, RoutedEventArgs const&)
	{
		GameState::XFactorStatModifiers.clear();

		std::wstring mentality;
		std::wstring physical;
		std::wstring weaknesses;

		if (m_selectedMentalityIndex >= 0 && m_selectedMentalityIndex < static_cast<int>(m_mentalityOptions.size()))
		{
			auto const& opt = m_mentalityOptions[m_selectedMentalityIndex];
			mentality = opt.Title;
			for (auto const& kv : opt.StatDeltas)
			{
				GameState::XFactorStatModifiers[kv.first] += kv.second;
			}
		}

		if (m_selectedPhysicalIndex >= 0 && m_selectedPhysicalIndex < static_cast<int>(m_physicalOptions.size()))
		{
			auto const& opt = m_physicalOptions[m_selectedPhysicalIndex];
			physical = opt.Title;
			for (auto const& kv : opt.StatDeltas)
			{
				GameState::XFactorStatModifiers[kv.first] += kv.second;
			}
		}

		if (WeaknessKick().IsChecked() && WeaknessKick().IsChecked().Value())
			weaknesses += L"Inconsistent Kicking, ";
		if (WeaknessFitness().IsChecked() && WeaknessFitness().IsChecked().Value())
			weaknesses += L"Low Endurance, ";
		if (WeaknessDecision().IsChecked() && WeaknessDecision().IsChecked().Value())
			weaknesses += L"Poor Decision Making, ";
		if (WeaknessPressure().IsChecked() && WeaknessPressure().IsChecked().Value())
			weaknesses += L"Struggles Under Pressure, ";

		if (!weaknesses.empty())
		{
			weaknesses.erase(weaknesses.size() - 2);
		}

		GameState::CurrentPlayer.mentalityXFactor = mentality;
		GameState::CurrentPlayer.physicalXFactor = physical;
		GameState::CurrentPlayer.weaknesses = weaknesses;

		Frame().Navigate(
			winrt::Windows::UI::Xaml::Interop::TypeName{
				L"thefootballife.ConfirmPlayerPage",
				winrt::Windows::UI::Xaml::Interop::TypeKind::Custom
			}
		);
	}
}
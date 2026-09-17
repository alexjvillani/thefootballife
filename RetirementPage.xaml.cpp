#include "pch.h"
#include "RetirementPage.xaml.h"
#include "GameState.h"
#if __has_include("RetirementPage.g.cpp")
#include "RetirementPage.g.cpp"
#include <winrt/Windows.UI.Xaml.Interop.h>
#endif

#include <algorithm>

using namespace winrt;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;

namespace
{
	// Duplicates CareerHubPage's/DraftNightPage's tier-name matching (same
	// known limitation noted there: best-effort string matching against a
	// small known list, since there's no explicit tier field on PlayerData
	// yet). Kept separate rather than shared since this page doesn't
	// otherwise share a header with those.
	enum class Tier { Local, TalentLeague, StateLeague, Afl };

	Tier DetermineTier(std::wstring const& league)
	{
		if (league == L"AFL") return Tier::Afl;
		if (league == L"VFL" || league == L"SANFL" || league == L"WAFL") return Tier::StateLeague;
		if (league == L"Talent League" || league == L"NAB League") return Tier::TalentLeague;
		return Tier::Local;
	}

	struct OverallRange { int Min{ 0 }; int Max{ 0 }; };

	OverallRange GetTierOverallRange(Tier tier)
	{
		switch (tier)
		{
		case Tier::Local:        return { 35, 60 };
		case Tier::TalentLeague: return { 45, 70 };
		case Tier::StateLeague:  return { 55, 79 };
		case Tier::Afl:          return { 65, 99 };
		}
		return { 35, 60 };
	}

	// Mirrors CareerHubPage::ComputePlayerOverall, but reads from a
	// PersonalStats snapshot (synced into GameState by RetireButton_Click)
	// rather than live page members, since this is a different page.
	int ComputeFinalOverall(SaveGameService::PersonalStats const& stats, Tier tier)
	{
		auto range = GetTierOverallRange(tier);
		int baseline = (range.Min + range.Max) / 2;
		int formModifier = ((stats.confidence + stats.discipline + stats.motivation) / 3 - 50) / 3;
		int fatigueStressPenalty = (stats.fatigue + stats.stress) / 20;

		int talentBonus = 0;
		for (auto const& kv : GameState::XFactorStatModifiers)
		{
			talentBonus += kv.second;
		}
		talentBonus /= 4;

		return std::clamp(baseline + formModifier - fatigueStressPenalty + talentBonus, range.Min, range.Max);
	}

	std::wstring ComputeCareerGrade(Tier tier, int bfWins)
	{
		switch (tier)
		{
		case Tier::Afl:          return bfWins > 0 ? L"AFL Champion" : L"AFL Footballer";
		case Tier::StateLeague:  return bfWins > 0 ? L"State League Star" : L"State League Footballer";
		case Tier::TalentLeague: return bfWins > 0 ? L"Talent League Standout" : L"Talent League Prospect";
		case Tier::Local:
		default:                 return bfWins > 0 ? L"Local Legend" : L"Local Footballer";
		}
	}

	// The payoff of the branching-narrative system: reads whichever story
	// flags were set across the career and builds a short epilogue
	// reflecting the choices actually made. Falls back to a generic line
	// if none of the curated arcs (Mentor, Family vs Football) ever
	// triggered - RNG-gated day events mean not every career sees them.
	std::wstring GenerateEpilogue(std::unordered_set<std::wstring> const& flags)
	{
		std::wstring epilogue;

		if (flags.count(L"mentor_legacy"))
		{
			epilogue += L"The lessons your mentor gave you never left - long after you moved on, you still play like someone's watching, because once, someone was. ";
		}
		else if (flags.count(L"mentor_bond_strong"))
		{
			epilogue += L"You never forgot the mentor who believed in you before you believed in yourself. ";
		}
		else if (flags.count(L"independent_resolve"))
		{
			epilogue += L"You forged your own path without a mentor's hand to guide you - and proved it could be done. ";
		}
		else if (flags.count(L"mentor_declined"))
		{
			epilogue += L"You always wondered, quietly, what might have been different with a mentor by your side. ";
		}

		if (flags.count(L"family_reconciled"))
		{
			epilogue += L"Whatever football took from your family along the way, you found your way back to them before it was too late. ";
		}
		else if (flags.count(L"family_estranged"))
		{
			epilogue += L"Football gave you everything you dreamed of - but the empty seats where family used to sit never quite stopped mattering. ";
		}
		else if (flags.count(L"family_strong_bond"))
		{
			epilogue += L"Your family stood behind you through every high and low, and you never forgot it. ";
		}
		else if (flags.count(L"chose_family_over_football") || flags.count(L"chose_football_over_family"))
		{
			epilogue += L"Football and family pulled at you in different directions more than once, and you made your choices as they came. ";
		}

		if (epilogue.empty())
		{
			epilogue = L"Yours was a quiet career, defined less by defining moments than by simply showing up, week after week, season after season. ";
		}

		return epilogue;
	}
}

namespace winrt::thefootballife::implementation
{
	RetirementPage::RetirementPage()
	{
		InitializeComponent();

		auto const& player = GameState::CurrentPlayer;
		auto const& stats = GameState::CurrentPersonalStats;
		Tier tier = DetermineTier(player.currentLeague);

		PlayerNameSubtitleText().Text(hstring(player.firstName + L" " + player.lastName + L" calls time on a storied career."));

		std::wstring grade = ComputeCareerGrade(tier, stats.careerBestAndFairestWins);
		CareerGradeText().Text(hstring(grade));

		std::wstring finalClub = player.team.empty() ? L"" : (L"Retired from " + player.team);
		if (!player.currentLeague.empty())
		{
			finalClub += finalClub.empty() ? (L"Played in the " + player.currentLeague) : (L" (" + player.currentLeague + L")");
		}
		FinalClubText().Text(hstring(finalClub));

		GamesPlayedText().Text(hstring(std::to_wstring(stats.gamesPlayed)));
		SeasonsPlayedText().Text(hstring(std::to_wstring(stats.careerSeasonsPlayed)));
		BestAndFairestText().Text(hstring(std::to_wstring(stats.careerBestAndFairestWins)));
		FinalOverallText().Text(hstring(std::to_wstring(ComputeFinalOverall(stats, tier))));

		EpilogueText().Text(hstring(GenerateEpilogue(GameState::StoryFlags)));
	}

	hstring RetirementPage::PageTitle() { return m_pageTitle; }
	void RetirementPage::PageTitle(hstring const& value) { m_pageTitle = value; }

	void RetirementPage::ReturnToMainMenuButton_Click(IInspectable const&, RoutedEventArgs const&)
	{
		Frame().Navigate(
			winrt::Windows::UI::Xaml::Interop::TypeName{
				L"thefootballife.MainMenuPage",
				winrt::Windows::UI::Xaml::Interop::TypeKind::Custom
			}
		);
	}
}
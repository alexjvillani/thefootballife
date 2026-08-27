#pragma once
#include "CareerHubPage.g.h"
#include "SaveGameService.h"
#include "FixtureService.h"
#include "DayEventService.h"
#include "SquadService.h"
#include <winrt/Windows.UI.h>
#include <vector>
#include <string>
#include <unordered_map>
#include <random>

namespace winrt::thefootballife::implementation
{
	struct CareerHubPage : CareerHubPageT<CareerHubPage>
	{
		CareerHubPage();

		winrt::hstring PageTitle();
		void PageTitle(winrt::hstring const& value);

		void TrainButton_Click(
			winrt::Windows::Foundation::IInspectable const& sender,
			winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);

		void PlayMatchButton_Click(
			winrt::Windows::Foundation::IInspectable const& sender,
			winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);

		void RestButton_Click(
			winrt::Windows::Foundation::IInspectable const& sender,
			winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);

		void StudyButton_Click(
			winrt::Windows::Foundation::IInspectable const& sender,
			winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);

		void IncrementBlockButton_Click(
			winrt::Windows::Foundation::IInspectable const& sender,
			winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);

		void DecrementBlockButton_Click(
			winrt::Windows::Foundation::IInspectable const& sender,
			winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);

		void SaveGameButton_Click(
			winrt::Windows::Foundation::IInspectable const& sender,
			winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);

		void ExitToMyCareerButton_Click(
			winrt::Windows::Foundation::IInspectable const& sender,
			winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);

		void AdvanceWeekButton_Click(
			winrt::Windows::Foundation::IInspectable const& sender,
			winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);

		void ExitToMainMenuButton_Click(
			winrt::Windows::Foundation::IInspectable const& sender,
			winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);

		void LadderViewButton_Click(
			winrt::Windows::Foundation::IInspectable const& sender,
			winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);

		void FixturesViewButton_Click(
			winrt::Windows::Foundation::IInspectable const& sender,
			winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);

		void SquadViewButton_Click(
			winrt::Windows::Foundation::IInspectable const& sender,
			winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);

		void NextSeasonButton_Click(
			winrt::Windows::Foundation::IInspectable const& sender,
			winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);

	private:
		struct LadderEntry
		{
			std::wstring clubName;
			std::wstring homeGround;
			int wins{ 0 };
			int losses{ 0 };
			int draws{ 0 };
			int pointsFor{ 0 };
			int pointsAgainst{ 0 };
			int played()       const { return wins + losses + draws; }
			int ladderPoints() const { return wins * 4 + draws * 2; }
			double percentage() const
			{
				return (pointsAgainst == 0)
					? (pointsFor > 0 ? 999.9 : 100.0)
					: (static_cast<double>(pointsFor) / pointsAgainst) * 100.0;
			}
		};

		void LoadPlayerData();
		void UpdateWeekDisplay();
		void UpdateBlockUI();
		void UpdateStateUI();
		void ApplyWeekSimulation();
		void SimulateWeekMatches(int playerClubBonus = 0, int opponentPenalty = 0);
		void ShowPreMatchDialog();
		void ResolveMatchday(int playerClubBonus, int opponentPenalty, winrt::hstring const& hintMessage);
		void AdjustBlockByTag(winrt::hstring const& tag, int delta);
		int  BlocksUsed() const;
		winrt::hstring FormatHeightFeet(int totalCm);
		void LoadLadderFromCsv();
		void RenderLadder();
		void RenderFixtures();
		void RenderSquad();
		int ComputePlayerOverall() const;

		// Real-world AFL pathway: Local -> Talent League -> VFL/SANFL/WAFL -> AFL

		enum class CompetitionTier { Local, TalentLeague, StateLeague, Afl };
		struct OverallRange { int Min{ 0 }; int Max{ 0 }; };
		CompetitionTier DetermineTier(std::wstring const& league) const;
		OverallRange GetTierOverallRange(CompetitionTier tier) const;

		winrt::Windows::UI::Color OverallColour(int overall, OverallRange const& range) const;
		void ShowDayEventDialog(DayEventService::DayEvent const& event);
		void ApplyEventChoice(DayEventService::EventChoice const& choice);
		void CheckForFinalsProgression();
		void ShowFinalsAnnouncementDialog(winrt::hstring const& title, winrt::hstring const& message);

		// One quarter's raw goals/behinds for both sides of a single match -
		// AFL scoring, not a running total (RenderLastMatchSummary computes
		// the cumulative view for display).
		struct QuarterScore
		{
			int homeGoals{ 0 };
			int homeBehinds{ 0 };
			int awayGoals{ 0 };
			int awayBehinds{ 0 };
		};
		std::vector<QuarterScore> GenerateMatchQuarters(std::mt19937& gen) const;
		void ApplyPointAdjustment(int& goals, int& behinds, int delta) const;
		std::wstring FormatAflScore(int goals, int behinds) const;
		void RenderLastMatchSummary();

		// Result of advancing exactly one day, used to drive the auto-advance
		// loop in AdvanceWeekButton_Click. A single day-step behaves the same
		// whether auto-advance is on or off - only the looping differs.
		enum class DayStepResult
		{
			NeedsBlocksBeforeFriday, // Blocked: Friday's 14 blocks aren't fully allocated yet
			StoppedAtKeyDay,         // Matchday (dialog or bye) or a day event fired - always halt here
			SeasonOver,              // Season Over marker present - calendar is frozen until Start Next Season
			Continue                 // Nothing needed player attention - safe to keep auto-advancing
		};
		DayStepResult AdvanceSingleDayStep();
		bool IsSeasonOver() const;
		void UpdateSeasonRolloverUI();

		// Pure projection of what the current block allocation would produce
		// if the week were committed right now. Shared by ApplyWeekSimulation
		// (which commits it) and the live preview shown while allocating.
		struct ProjectedStats
		{
			int fatigue{ 0 };
			int injuryRisk{ 0 };
			int recoveryQuality{ 0 };
			int confidence{ 0 };
			int stress{ 0 };
			int motivation{ 0 };
			int discipline{ 0 };
			int finances{ 0 };
			int relationships{ 0 };
		};
		ProjectedStats ComputeProjectedStats() const;

	private:
		winrt::hstring m_pageTitle{ L"Career Hub" };

		int m_currentWeek{ 1 };
		winrt::hstring m_lastChoice{ L"No action chosen yet." };

		static constexpr int kTotalBlocks{ 14 };
		static constexpr int kBlocksPerDay{ 3 }; // Mon-Thu cap; Friday has no cap but must reach kTotalBlocks
		int m_trainingBlocks{ 4 };
		int m_schoolBlocks{ 5 };
		int m_workBlocks{ 2 };
		int m_socialBlocks{ 2 };
		int m_recoveryBlocks{ 1 };
		int m_blocksSpentToday{ 0 }; // resets each day; equals sum of m_blocksAddedTodayByCategory
		std::unordered_map<std::wstring, int> m_blocksAddedTodayByCategory; // per-category "added today" so decrementing PREVIOUS days' stock doesn't falsely free up today's cap

		int m_fatigue{ 30 };
		int m_injuryRisk{ 20 };
		int m_recoveryQuality{ 55 };
		int m_confidence{ 55 };
		int m_stress{ 35 };
		int m_motivation{ 60 };
		int m_discipline{ 60 };
		int m_finances{ 35 };
		int m_relationships{ 50 };

		std::unordered_map<std::wstring, SaveGameService::TeamSeasonStats> m_teamStats;
		std::vector<LadderEntry> m_ladder;
		std::vector<FixtureService::Fixture> m_fixtures;

		// Teammate roster for the Squad view - generated once per season
		// (lazily on first render, regenerated on Start Next Season). Not
		// persisted to save files, same as the personal stats it partly
		// depends on via ComputePlayerOverall.
		std::vector<SquadService::SquadMember> m_squad;

		static constexpr int kDayEventChancePercent{ 20 }; // per Mon-Fri day advance
		std::vector<DayEventService::DayEvent> m_dayEvents;

		// Last match's quarter-by-quarter breakdown, for the "down the
		// bottom" summary panel. Empty until the player's first match of
		// the career; deliberately not persisted to save files (same as
		// BottomHintText/ConsequenceText - transient session display only).
		std::vector<QuarterScore> m_lastMatchQuarters;
		std::wstring m_lastMatchHomeClub;
		std::wstring m_lastMatchAwayClub;
	};
}

namespace winrt::thefootballife::factory_implementation
{
	struct CareerHubPage : CareerHubPageT<CareerHubPage, implementation::CareerHubPage>
	{
	};
}
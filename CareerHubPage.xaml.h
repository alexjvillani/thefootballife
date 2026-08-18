#pragma once
#include "CareerHubPage.g.h"
#include "SaveGameService.h"
#include "FixtureService.h"
#include "DayEventService.h"
#include <vector>
#include <string>
#include <unordered_map>

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
		void ShowDayEventDialog(DayEventService::DayEvent const& event);
		void ApplyEventChoice(DayEventService::EventChoice const& choice);
		void CheckForFinalsProgression();
		void ShowFinalsAnnouncementDialog(winrt::hstring const& title, winrt::hstring const& message);

		// Result of advancing exactly one day, used to drive the auto-advance
		// loop in AdvanceWeekButton_Click. A single day-step behaves the same
		// whether auto-advance is on or off - only the looping differs.
		enum class DayStepResult
		{
			NeedsBlocksBeforeFriday, // Blocked: Friday's 14 blocks aren't fully allocated yet
			StoppedAtKeyDay,         // Matchday (dialog or bye) or a day event fired - always halt here
			Continue                 // Nothing needed player attention - safe to keep auto-advancing
		};
		DayStepResult AdvanceSingleDayStep();

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

		static constexpr int kDayEventChancePercent{ 20 }; // per Mon-Fri day advance
		std::vector<DayEventService::DayEvent> m_dayEvents;
	};
}

namespace winrt::thefootballife::factory_implementation
{
	struct CareerHubPage : CareerHubPageT<CareerHubPage, implementation::CareerHubPage>
	{
	};
}
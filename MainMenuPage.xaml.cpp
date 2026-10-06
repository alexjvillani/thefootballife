#include "pch.h"
#include "MainMenuPage.xaml.h"
#if __has_include("MainMenuPage.g.cpp")
#include "MainMenuPage.g.cpp"
#endif

#include "PlayerCreationPage.xaml.h"
#include "CareerHubPage.xaml.h"
#include "CreditPage.xaml.h"
#include "GameState.h"
#include "SaveGameService.h"
#include <unordered_set>
#include <vector>
#include <winrt/Windows.UI.Xaml.Interop.h>

using namespace winrt;
using namespace Windows::Foundation;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;

namespace
{
	// One row in the Load Game list: either a manual slot (1..MaxSaveSlots)
	// or an autosave (0 = "latest" recovery save, 1..MaxAutosaves = rolling
	// milestone history, 1 = newest).
	struct SaveEntry
	{
		bool isAutosave{ false };
		int index{ 0 };
	};

	std::vector<SaveEntry> BuildSaveEntries()
	{
		std::vector<SaveEntry> entries;
		for (int i = 0; i <= SaveGameService::MaxAutosaves; ++i)
		{
			entries.push_back(SaveEntry{ true, i });
		}
		for (int slot = 1; slot <= SaveGameService::MaxSaveSlots; ++slot)
		{
			entries.push_back(SaveEntry{ false, slot });
		}
		return entries;
	}

	bool EntryExists(SaveEntry const& e)
	{
		return e.isAutosave ? SaveGameService::AutosaveExists(e.index)
			: SaveGameService::SlotExists(e.index);
	}

	bool EntryPreview(SaveEntry const& e, std::wstring& playerName, int& week)
	{
		return e.isAutosave ? SaveGameService::GetAutosavePreview(e.index, playerName, week)
			: SaveGameService::GetSavePreview(e.index, playerName, week);
	}

	std::wstring EntryName(SaveEntry const& e)
	{
		if (e.isAutosave)
		{
			return e.index == 0 ? std::wstring(L"Latest Autosave")
				: L"Autosave " + std::to_wstring(e.index);
		}
		return L"Slot " + std::to_wstring(e.index);
	}
}

namespace winrt::thefootballife::implementation
{
	MainMenuPage::MainMenuPage()
	{
		InitializeComponent();
	}

	void MainMenuPage::NewGame_Click(IInspectable const&, RoutedEventArgs const&)
	{
		Frame().Navigate(
			winrt::Windows::UI::Xaml::Interop::TypeName{
				L"thefootballife.PlayerCreationPage",
				winrt::Windows::UI::Xaml::Interop::TypeKind::Custom
			}
		);
	}

	void MainMenuPage::LoadGame_Click(IInspectable const&, RoutedEventArgs const&)
	{
		std::vector<SaveEntry> const entries = BuildSaveEntries();

		bool hasAnySave = false;
		for (auto const& e : entries)
		{
			if (EntryExists(e))
			{
				hasAnySave = true;
				break;
			}
		}

		if (!hasAnySave)
		{
			ContentDialog dialog;
			dialog.Title(box_value(L"No Saves Found"));
			dialog.Content(box_value(L"No save files exist yet. Create and save a career first."));
			dialog.CloseButtonText(L"OK");
			dialog.XamlRoot(this->XamlRoot());
			dialog.ShowAsync();
			return;
		}

		ComboBox slotComboBox;
		int firstAvailable = -1;

		for (size_t i = 0; i < entries.size(); ++i)
		{
			SaveEntry const& e = entries[i];
			ComboBoxItem item;
			std::wstring label = EntryName(e);
			bool available = false;

			if (EntryExists(e))
			{
				std::wstring playerName;
				int week = 1;

				if (EntryPreview(e, playerName, week))
				{
					label += L" - " + playerName + L" (Week " + std::to_wstring(week) + L")";

					if (e.isAutosave)
					{
						std::wstring when = SaveGameService::GetAutosaveTimeLabel(e.index);
						if (!when.empty())
						{
							label += L" - " + when;
						}
					}
					available = true;
				}
			}

			if (!available)
			{
				label += L" - Not Available";
				item.IsEnabled(false);
			}
			else if (firstAvailable < 0)
			{
				firstAvailable = static_cast<int>(i);
			}

			item.Content(box_value(hstring(label)));
			slotComboBox.Items().Append(item);
		}

		// Defaults to the first available entry - the "Latest Autosave" when
		// one exists, since it's always the most recent state of the career.
		if (firstAvailable >= 0)
		{
			slotComboBox.SelectedIndex(firstAvailable);
		}

		ContentDialog dialog;
		dialog.Title(box_value(L"Load Game"));
		dialog.Content(slotComboBox);
		dialog.PrimaryButtonText(L"Load");
		dialog.SecondaryButtonText(L"Delete");
		dialog.CloseButtonText(L"Cancel");
		dialog.XamlRoot(this->XamlRoot());

		auto weakThis = get_weak();
		dialog.ShowAsync().Completed(
			[weakThis, slotComboBox, entries](auto const& operation, auto const&)
			{
				if (auto self = weakThis.get())
				{
					ContentDialogResult result = operation.GetResults();
					int selected = static_cast<int>(slotComboBox.SelectedIndex());

					if (selected < 0 || selected >= static_cast<int>(entries.size()))
						return;

					SaveEntry const entry = entries[selected];

					if (result == ContentDialogResult::Primary)
					{
						PlayerData loadedPlayer;
						int loadedWeek = 1;
						std::wstring loadedChoice;
						std::unordered_map<std::wstring, SaveGameService::TeamSeasonStats> loadedTeamStats;
						std::vector<FixtureService::Fixture> fixtures;
						SaveGameService::CalendarState loadedCalendar;
						SaveGameService::PersonalStats loadedPersonalStats;
						std::unordered_set<std::wstring> loadedStoryFlags;

						bool loaded = entry.isAutosave
							? SaveGameService::LoadAutosave(
								entry.index,
								loadedPlayer,
								loadedWeek,
								loadedChoice,
								loadedTeamStats,
								fixtures,
								loadedCalendar,
								loadedPersonalStats,
								loadedStoryFlags)
							: SaveGameService::LoadFromSlot(
								entry.index,
								loadedPlayer,
								loadedWeek,
								loadedChoice,
								loadedTeamStats,
								fixtures,
								loadedCalendar,
								loadedPersonalStats,
								loadedStoryFlags);

						if (!loaded)
						{
							ContentDialog failDialog;
							failDialog.Title(box_value(L"Load Failed"));
							failDialog.Content(box_value(L"Could not read the selected save."));
							failDialog.CloseButtonText(L"OK");
							failDialog.XamlRoot(self->XamlRoot());
							failDialog.ShowAsync();
							return;
						}

						GameState::CurrentPlayer = loadedPlayer;
						GameState::CurrentWeek = loadedWeek;
						GameState::LastChoice = loadedChoice;
						GameState::TeamStats = loadedTeamStats;
						GameState::Fixtures = fixtures;
						GameState::CurrentPersonalStats = loadedPersonalStats;
						GameState::StoryFlags = loadedStoryFlags;

						GameState::CurrentDate = SimpleDate{
							loadedCalendar.currentYear,
							loadedCalendar.currentMonth,
							loadedCalendar.currentDay
						};
						GameState::CurrentDay = static_cast<DayPhase>(loadedCalendar.currentDayPhase);
						GameState::SeasonStartDate = SimpleDate{
							loadedCalendar.seasonStartYear,
							loadedCalendar.seasonStartMonth,
							loadedCalendar.seasonStartDay
						};
						GameState::SeasonEndDate = SimpleDate{
							loadedCalendar.seasonEndYear,
							loadedCalendar.seasonEndMonth,
							loadedCalendar.seasonEndDay
						};

						self->Frame().Navigate(
							winrt::Windows::UI::Xaml::Interop::TypeName{
								L"thefootballife.CareerHubPage",
								winrt::Windows::UI::Xaml::Interop::TypeKind::Custom
							}
						);
					}
					else if (result == ContentDialogResult::Secondary)
					{
						ContentDialog confirmDialog;
						confirmDialog.Title(box_value(L"Delete Save"));
						confirmDialog.Content(box_value(L"Are you sure you want to delete this save?"));
						confirmDialog.PrimaryButtonText(L"Delete");
						confirmDialog.CloseButtonText(L"Cancel");
						confirmDialog.XamlRoot(self->XamlRoot());

						auto weakSelf2 = self->get_weak();
						confirmDialog.ShowAsync().Completed(
							[weakSelf2, entry](auto const& confirmOperation, auto const&)
							{
								if (auto self2 = weakSelf2.get())
								{
									if (confirmOperation.GetResults() != ContentDialogResult::Primary)
										return;

									bool deleted = entry.isAutosave
										? SaveGameService::DeleteAutosave(entry.index)
										: SaveGameService::DeleteSlot(entry.index);

									ContentDialog resultDialog;
									resultDialog.XamlRoot(self2->XamlRoot());

									if (deleted)
									{
										resultDialog.Title(box_value(L"Save Deleted"));
										resultDialog.Content(box_value(L"The selected save was deleted."));
									}
									else
									{
										resultDialog.Title(box_value(L"Delete Failed"));
										resultDialog.Content(box_value(L"Could not delete the selected save."));
									}

									resultDialog.CloseButtonText(L"OK");
									resultDialog.ShowAsync();
								}
							}
						);
					}
				}
			}
		);
	}

	void MainMenuPage::Settings_Click(IInspectable const&, RoutedEventArgs const&)
	{
		ContentDialog dialog;
		dialog.Title(box_value(L"Settings"));
		dialog.Content(box_value(L"Settings menu coming soon."));
		dialog.CloseButtonText(L"OK");
		dialog.XamlRoot(this->XamlRoot());
		dialog.ShowAsync();
	}

	void MainMenuPage::Credits_Click(IInspectable const&, RoutedEventArgs const&)
	{
		Frame().Navigate(
			winrt::Windows::UI::Xaml::Interop::TypeName{
				L"thefootballife.CreditPage",
				winrt::Windows::UI::Xaml::Interop::TypeKind::Custom
			}
		);
	}

	void MainMenuPage::Exit_Click(IInspectable const&, RoutedEventArgs const&)
	{
		Application::Current().Exit();
	}
}
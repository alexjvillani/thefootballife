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
#include <winrt/Windows.UI.Xaml.Interop.h>

using namespace winrt;
using namespace Windows::Foundation;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;

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
		bool hasAnySave = false;
		for (int slot = 1; slot <= SaveGameService::MaxSaveSlots; ++slot)
		{
			if (SaveGameService::SlotExists(slot))
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

		for (int slot = 1; slot <= SaveGameService::MaxSaveSlots; ++slot)
		{
			ComboBoxItem item;
			std::wstring label = L"Slot " + std::to_wstring(slot);

			if (SaveGameService::SlotExists(slot))
			{
				std::wstring playerName;
				int week = 1;

				if (SaveGameService::GetSavePreview(slot, playerName, week))
				{
					label += L" - " + playerName + L" (Week " + std::to_wstring(week) + L")";
				}
				else
				{
					label += L" - Not Available";
					item.IsEnabled(false);
				}
			}
			else
			{
				label += L" - Not Available";
				item.IsEnabled(false);
			}

			item.Content(box_value(hstring(label)));
			slotComboBox.Items().Append(item);
		}

		for (int slot = 1; slot <= SaveGameService::MaxSaveSlots; ++slot)
		{
			if (SaveGameService::SlotExists(slot))
			{
				slotComboBox.SelectedIndex(slot - 1);
				break;
			}
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
			[weakThis, slotComboBox](auto const& operation, auto const&)
			{
				if (auto self = weakThis.get())
				{
					ContentDialogResult result = operation.GetResults();
					int slot = static_cast<int>(slotComboBox.SelectedIndex()) + 1;

					if (slot < 1 || slot > SaveGameService::MaxSaveSlots)
						return;

					if (result == ContentDialogResult::Primary)
					{
						PlayerData loadedPlayer;
						int loadedWeek = 1;
						std::wstring loadedChoice;
						std::unordered_map<std::wstring, SaveGameService::TeamSeasonStats> loadedTeamStats;
						std::vector<FixtureService::Fixture> fixtures;
						SaveGameService::CalendarState loadedCalendar;
						SaveGameService::PersonalStats loadedPersonalStats;

						bool loaded = SaveGameService::LoadFromSlot(
							slot,
							loadedPlayer,
							loadedWeek,
							loadedChoice,
							loadedTeamStats,
							fixtures,
							loadedCalendar,
							loadedPersonalStats
						);

						if (!loaded)
						{
							ContentDialog failDialog;
							failDialog.Title(box_value(L"Load Failed"));
							failDialog.Content(box_value(L"Could not read the selected save slot."));
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
							[weakSelf2, slot](auto const& confirmOperation, auto const&)
							{
								if (auto self2 = weakSelf2.get())
								{
									if (confirmOperation.GetResults() != ContentDialogResult::Primary)
										return;

									bool deleted = SaveGameService::DeleteSlot(slot);

									ContentDialog resultDialog;
									resultDialog.XamlRoot(self2->XamlRoot());

									if (deleted)
									{
										resultDialog.Title(box_value(L"Save Deleted"));
										resultDialog.Content(box_value(L"The selected save slot was deleted."));
									}
									else
									{
										resultDialog.Title(box_value(L"Delete Failed"));
										resultDialog.Content(box_value(L"Could not delete the selected save slot."));
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
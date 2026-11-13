#include "pch.h"
#include "SettingsPage.xaml.h"
#if __has_include("SettingsPage.g.cpp")
#include "SettingsPage.g.cpp"
#endif

#include "DisplaySettings.h"

#include <string>

#include <winrt/Windows.UI.Xaml.Interop.h>

using namespace winrt;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;

namespace winrt::thefootballife::implementation
{
	SettingsPage::SettingsPage()
	{
		InitializeComponent();

		// The choices come from DisplaySettings so the page and the settings
		// code can never disagree about what index N means.
		WindowModeComboBox().Items().Append(box_value(L"Maximised"));
		WindowModeComboBox().Items().Append(box_value(L"Windowed"));
		WindowModeComboBox().Items().Append(box_value(L"Fullscreen"));

		for (int i = 0; i < DisplaySettings::kWindowSizePresetCount; ++i)
		{
			auto const& preset = DisplaySettings::kWindowSizePresets[i];
			std::wstring label = std::to_wstring(preset.width) + L" \u00D7 " + std::to_wstring(preset.height);
			WindowSizeComboBox().Items().Append(box_value(hstring(label)));
		}

		for (int i = 0; i < DisplaySettings::kUiScaleChoiceCount; ++i)
		{
			int const percent = DisplaySettings::kUiScaleChoices[i];
			std::wstring label = (percent == 0) ? std::wstring(L"Auto (fit to window)") : (std::to_wstring(percent) + L"%");
			UiScaleComboBox().Items().Append(box_value(hstring(label)));
		}

		LoadControlsFromSettings();
	}

	void SettingsPage::LoadControlsFromSettings()
	{
		DisplaySettings::Values const& v = DisplaySettings::Current();

		WindowModeComboBox().SelectedIndex(static_cast<int>(v.mode));

		int sizeIndex = DisplaySettings::kDefaultWindowSizePreset;
		for (int i = 0; i < DisplaySettings::kWindowSizePresetCount; ++i)
		{
			if (DisplaySettings::kWindowSizePresets[i].width == v.windowWidth &&
				DisplaySettings::kWindowSizePresets[i].height == v.windowHeight)
			{
				sizeIndex = i;
				break;
			}
		}
		WindowSizeComboBox().SelectedIndex(sizeIndex);

		int scaleIndex = 0; // Auto
		for (int i = 0; i < DisplaySettings::kUiScaleChoiceCount; ++i)
		{
			if (DisplaySettings::kUiScaleChoices[i] == v.uiScalePercent)
			{
				scaleIndex = i;
				break;
			}
		}
		UiScaleComboBox().SelectedIndex(scaleIndex);

		UpdateWindowSizeEnabled();
	}

	// Window size only means something in Windowed mode.
	void SettingsPage::UpdateWindowSizeEnabled()
	{
		bool const windowed = (WindowModeComboBox().SelectedIndex() == static_cast<int>(DisplaySettings::WindowMode::Windowed));
		WindowSizeComboBox().IsEnabled(windowed);
	}

	void SettingsPage::WindowModeComboBox_SelectionChanged(
		IInspectable const&, SelectionChangedEventArgs const&)
	{
		UpdateWindowSizeEnabled();
	}

	void SettingsPage::ApplyButton_Click(IInspectable const&, RoutedEventArgs const&)
	{
		DisplaySettings::Values v = DisplaySettings::Current();

		int const modeIndex = WindowModeComboBox().SelectedIndex();
		if (modeIndex >= 0 && modeIndex <= 2)
		{
			v.mode = static_cast<DisplaySettings::WindowMode>(modeIndex);
		}

		int const sizeIndex = WindowSizeComboBox().SelectedIndex();
		if (sizeIndex >= 0 && sizeIndex < DisplaySettings::kWindowSizePresetCount)
		{
			v.windowWidth = DisplaySettings::kWindowSizePresets[sizeIndex].width;
			v.windowHeight = DisplaySettings::kWindowSizePresets[sizeIndex].height;
		}

		int const scaleIndex = UiScaleComboBox().SelectedIndex();
		if (scaleIndex >= 0 && scaleIndex < DisplaySettings::kUiScaleChoiceCount)
		{
			v.uiScalePercent = DisplaySettings::kUiScaleChoices[scaleIndex];
		}

		bool const saved = DisplaySettings::Apply(v);
		StatusText().Text(saved ? L"Applied and saved." : L"Applied, but the settings file could not be saved.");
	}

	void SettingsPage::ResetButton_Click(IInspectable const&, RoutedEventArgs const&)
	{
		bool const saved = DisplaySettings::Apply(DisplaySettings::Values{});
		LoadControlsFromSettings();
		StatusText().Text(saved ? L"Reset to defaults." : L"Reset, but the settings file could not be saved.");
	}

	void SettingsPage::BackButton_Click(IInspectable const&, RoutedEventArgs const&)
	{
		Frame().Navigate(winrt::Windows::UI::Xaml::Interop::TypeName{
			L"thefootballife.MainMenuPage",
			winrt::Windows::UI::Xaml::Interop::TypeKind::Custom });
	}
}
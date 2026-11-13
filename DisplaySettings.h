#pragma once

#include <winrt/Microsoft.UI.Xaml.h>
#include <winrt/Microsoft.UI.Windowing.h>

// Display options: window mode, window size and UI scale.
//
// Stored in "settings.txt" inside the same "The Football Life" folder as the
// save files (see SaveGameService::GetSaveFolder), so it needs no extra
// packaging or registry access.
//
// "Resolution" in a windowed WinUI app means two different things, and this
// covers both:
//   * Window size  - how big the window is, in physical pixels (Windowed mode).
//   * UI scale     - how big the layout is drawn inside the window. The pages
//                    were designed for 1920x1080, so "Auto" scales the whole
//                    layout up to fill larger windows and screens (e.g. 4K)
//                    instead of leaving dead space.
namespace DisplaySettings
{
	enum class WindowMode
	{
		Maximized = 0,
		Windowed = 1,
		Fullscreen = 2
	};

	struct SizePreset
	{
		int width;
		int height;
	};

	// Window size choices offered in Windowed mode (physical pixels, client area).
	constexpr SizePreset kWindowSizePresets[] = {
		{ 1280, 720 },
		{ 1600, 900 },
		{ 1920, 1080 },
		{ 2560, 1440 },
		{ 3840, 2160 } };
	constexpr int kWindowSizePresetCount = 5;
	constexpr int kDefaultWindowSizePreset = 2; // 1920 x 1080

	// UI scale choices. 0 means "Auto" (fit the 1920x1080 design to the window).
	constexpr int kUiScaleChoices[] = { 0, 100, 125, 150, 200 };
	constexpr int kUiScaleChoiceCount = 5;

	struct Values
	{
		WindowMode mode{ WindowMode::Maximized }; // matches the game's original start-up behaviour
		int windowWidth{ 1920 };
		int windowHeight{ 1080 };
		int uiScalePercent{ 0 };                  // 0 = Auto
	};

	Values const& Current();

	// Reads settings.txt. A missing or unreadable file just means defaults.
	void Load();

	// Remembers the main window and the two elements the UI scale acts on:
	// "host" is the window-sized grid, "content" is the frame that gets
	// resized and scaled inside it.
	void Attach(
		winrt::Microsoft::UI::Windowing::AppWindow const& appWindow,
		winrt::Microsoft::UI::Xaml::FrameworkElement const& host,
		winrt::Microsoft::UI::Xaml::FrameworkElement const& content);

	// Releases those references (call when the main window closes).
	void Detach();

	// Applies window mode / size from the current values.
	void ApplyWindow();

	// Recomputes the UI scale from the host's current size. Cheap; call it
	// from the host's SizeChanged event.
	void ApplyScale();

	// Makes `values` current, saves them, and applies them to the live window.
	// Returns false if the settings file could not be written (the changes
	// are still applied for this session).
	bool Apply(Values const& values);
}
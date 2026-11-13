#include "pch.h"
#include "DisplaySettings.h"
#include "SaveGameService.h"

#include <algorithm>
#include <fstream>
#include <limits>
#include <string>

#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Graphics.h>
#include <winrt/Microsoft.UI.Windowing.h>
#include <winrt/Microsoft.UI.Xaml.Media.h>

namespace
{
	using namespace DisplaySettings;

	Values g_values;
	winrt::Microsoft::UI::WindowId g_windowId{};
	bool g_hasWindow{ false };
	winrt::Microsoft::UI::Xaml::FrameworkElement g_hostElement{ nullptr };
	winrt::Microsoft::UI::Xaml::FrameworkElement g_contentElement{ nullptr };

	// The pages were laid out for a 1920x1080 window; "Auto" UI scale
	// stretches that layout to fit the real window.
	constexpr double kDesignWidth = 1920.0;
	constexpr double kDesignHeight = 1080.0;

	std::wstring SettingsPath()
	{
		std::wstring const folder = SaveGameService::GetSaveFolder();
		if (folder.empty())
		{
			return L"";
		}
		return folder + L"\\settings.txt";
	}

	// BEGIN PARSE
	// Plain "key=value" lines. Unknown keys and bad numbers are ignored, so a
	// hand-edited or older file can never stop the game starting.
	Values ParseSettings(std::istream& in)
	{
		Values v;
		std::string line;
		while (std::getline(in, line))
		{
			while (!line.empty() && (line.back() == '\r' || line.back() == '\n' || line.back() == ' '))
			{
				line.pop_back();
			}

			size_t const eq = line.find('=');
			if (eq == std::string::npos)
			{
				continue;
			}

			std::string const key = line.substr(0, eq);
			int n = 0;
			try
			{
				n = std::stoi(line.substr(eq + 1));
			}
			catch (...)
			{
				continue;
			}

			if (key == "window_mode")
			{
				if (n >= 0 && n <= 2) v.mode = static_cast<WindowMode>(n);
			}
			else if (key == "window_width")
			{
				v.windowWidth = (std::clamp)(n, 640, 7680);
			}
			else if (key == "window_height")
			{
				v.windowHeight = (std::clamp)(n, 480, 4320);
			}
			else if (key == "ui_scale_percent")
			{
				v.uiScalePercent = (n <= 0) ? 0 : (std::clamp)(n, 50, 300);
			}
		}
		return v;
	}
	// END PARSE
}

namespace DisplaySettings
{
	Values const& Current()
	{
		return g_values;
	}

	void Load()
	{
		g_values = Values{};

		std::wstring const path = SettingsPath();
		if (path.empty())
		{
			return;
		}

		std::ifstream in(path);
		if (!in.is_open())
		{
			return;
		}

		g_values = ParseSettings(in);
	}

	static bool Save()
	{
		std::wstring const path = SettingsPath();
		if (path.empty())
		{
			return false;
		}

		std::ofstream out(path, std::ios::trunc);
		if (!out.is_open())
		{
			return false;
		}

		out << "window_mode=" << static_cast<int>(g_values.mode) << "\n";
		out << "window_width=" << g_values.windowWidth << "\n";
		out << "window_height=" << g_values.windowHeight << "\n";
		out << "ui_scale_percent=" << g_values.uiScalePercent << "\n";
		out.flush();
		return out.good();
	}

	void Attach(
		winrt::Microsoft::UI::Windowing::AppWindow const& appWindow,
		winrt::Microsoft::UI::Xaml::FrameworkElement const& host,
		winrt::Microsoft::UI::Xaml::FrameworkElement const& content)
	{
		g_windowId = appWindow.Id();
		g_hasWindow = true;
		g_hostElement = host;
		g_contentElement = content;
	}

	void Detach()
	{
		g_hasWindow = false;
		g_hostElement = nullptr;
		g_contentElement = nullptr;
	}

	void ApplyWindow()
	{
		using namespace winrt::Microsoft::UI::Windowing;

		if (!g_hasWindow)
		{
			return;
		}

		AppWindow appWindow = AppWindow::GetFromWindowId(g_windowId);
		if (!appWindow)
		{
			return;
		}

		auto setPresenterKind = [&](AppWindowPresenterKind kind)
			{
				if (appWindow.Presenter().Kind() != kind)
				{
					appWindow.SetPresenter(kind);
				}
			};

		switch (g_values.mode)
		{
		case WindowMode::Fullscreen:
			setPresenterKind(AppWindowPresenterKind::FullScreen);
			break;

		case WindowMode::Maximized:
			setPresenterKind(AppWindowPresenterKind::Overlapped);
			if (auto presenter = appWindow.Presenter().try_as<OverlappedPresenter>())
			{
				presenter.Maximize();
			}
			break;

		case WindowMode::Windowed:
		default:
		{
			setPresenterKind(AppWindowPresenterKind::Overlapped);
			if (auto presenter = appWindow.Presenter().try_as<OverlappedPresenter>())
			{
				presenter.Restore();
			}

			// Keep the window inside the monitor it is on. The title bar and
			// borders are measured (outer size minus client size) so a
			// "3840 x 2160" request on a 4K screen is trimmed to what fits.
			DisplayArea const area = DisplayArea::GetFromWindowId(g_windowId, DisplayAreaFallback::Nearest);
			auto const work = area.WorkArea();
			auto const outer = appWindow.Size();
			auto const client = appWindow.ClientSize();
			int const extraWidth = outer.Width - client.Width;
			int const extraHeight = outer.Height - client.Height;

			int const width = (std::clamp)(g_values.windowWidth, 640, (std::max)(640, work.Width - extraWidth));
			int const height = (std::clamp)(g_values.windowHeight, 480, (std::max)(480, work.Height - extraHeight));

			appWindow.ResizeClient(winrt::Windows::Graphics::SizeInt32{ width, height });

			// Centre it on that monitor.
			auto const newOuter = appWindow.Size();
			appWindow.Move(winrt::Windows::Graphics::PointInt32{
				work.X + (std::max)(0, (work.Width - newOuter.Width) / 2),
				work.Y + (std::max)(0, (work.Height - newOuter.Height) / 2) });
			break;
		}
		}
	}

	void ApplyScale()
	{
		if (!g_hostElement || !g_contentElement)
		{
			return;
		}

		double const hostWidth = g_hostElement.ActualWidth();
		double const hostHeight = g_hostElement.ActualHeight();
		if (hostWidth < 1.0 || hostHeight < 1.0)
		{
			return; // not laid out yet - SizeChanged will call us again
		}

		double scale = 1.0;
		if (g_values.uiScalePercent > 0)
		{
			scale = g_values.uiScalePercent / 100.0;
		}
		else
		{
			// Auto: the largest scale at which the 1920x1080 design still fits.
			scale = (std::min)(hostWidth / kDesignWidth, hostHeight / kDesignHeight);

			// A maximised 1080p window is a little shorter than 1080 once the
			// title bar and taskbar are subtracted. Treat "nearly" as exactly
			// 100% so that screen looks the same as it always has.
			if (scale > 0.9 && scale < 1.0)
			{
				scale = 1.0;
			}

			scale = (std::clamp)(scale, 0.6, 4.0);
		}

		// The frame is laid out at (window size / scale) and then scaled up by
		// `scale`, so it exactly covers the window while the pages still lay
		// themselves out as if the screen were smaller.
		g_contentElement.Width(hostWidth / scale);
		g_contentElement.Height(hostHeight / scale);

		winrt::Microsoft::UI::Xaml::Media::ScaleTransform transform;
		transform.ScaleX(scale);
		transform.ScaleY(scale);
		g_contentElement.RenderTransformOrigin(winrt::Windows::Foundation::Point{ 0.0f, 0.0f });
		g_contentElement.RenderTransform(transform);
	}

	bool Apply(Values const& values)
	{
		g_values = values;
		bool const saved = Save();
		ApplyWindow();
		ApplyScale();
		return saved;
	}
}
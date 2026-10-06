#pragma once
#include <string>

namespace CrashLogger
{
	// Call once, as early as possible (App constructor).
	void Install();

	// Short breadcrumb included in every crash report, e.g. L"Round 7, Talent League".
	void SetContext(std::wstring const& text);

	// For the XAML UnhandledException event.
	void LogXamlException(wchar_t const* message, unsigned int hresult);

	// The folder crash logs are being written to (for an "Open logs folder" button).
	std::wstring GetLogDirectory();
}
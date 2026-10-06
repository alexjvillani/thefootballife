#include "pch.h"
#include <winrt/Windows.Storage.h>
#include "CrashLogger.h"
#include <windows.h>
#include <shlobj.h>
#include <knownfolders.h>
#include <dbghelp.h>
#include <cstdio>
#include <cwchar>
#include <exception>
#include <string>

#pragma comment(lib, "dbghelp.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "ole32.lib")


namespace
{
	wchar_t g_logDir[MAX_PATH] = {};
	wchar_t g_context[256] = {};
	volatile LONG g_reporting = 0; // guards against re-entrancy / double reports

	void Timestamp(wchar_t* buf, size_t count)
	{
		SYSTEMTIME st;
		GetLocalTime(&st);
		swprintf_s(buf, count, L"%04d%02d%02d-%02d%02d%02d",
			st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
	}

	bool TryUseLogDir(std::wstring const& dir)
	{
		// Creates all intermediate folders.
		int rc = SHCreateDirectoryExW(nullptr, dir.c_str(), nullptr);
		if (rc != ERROR_SUCCESS && rc != ERROR_ALREADY_EXISTS && rc != ERROR_FILE_EXISTS)
		{
			return false;
		}

		// Probe with a real write - Controlled Folder Access can allow the
		// mkdir but still block file creation.
		std::wstring probe = dir + L"\\.write-test";
		HANDLE h = CreateFileW(probe.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS,
			FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE, nullptr);
		if (h == INVALID_HANDLE_VALUE)
		{
			return false;
		}
		CloseHandle(h);
		return true;
	}

	void WriteMiniDump(EXCEPTION_POINTERS* ep, wchar_t const* path)
	{
		HANDLE file = CreateFileW(path, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
		if (file == INVALID_HANDLE_VALUE) return;

		MINIDUMP_EXCEPTION_INFORMATION info{};
		info.ThreadId = GetCurrentThreadId();
		info.ExceptionPointers = ep;
		info.ClientPointers = FALSE;

		MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), file, MiniDumpNormal,
			ep ? &info : nullptr, nullptr, nullptr);
		CloseHandle(file);
	}

	void WriteReport(wchar_t const* kind, wchar_t const* detail, DWORD code, void* address, EXCEPTION_POINTERS* ep)
	{
		if (InterlockedExchange(&g_reporting, 1) != 0) return;
		if (g_logDir[0] == L'\0') return;

		wchar_t stamp[32];
		Timestamp(stamp, 32);

		wchar_t txtPath[MAX_PATH];
		wchar_t dmpPath[MAX_PATH];
		swprintf_s(txtPath, L"%s\\crash-%s.txt", g_logDir, stamp);
		swprintf_s(dmpPath, L"%s\\crash-%s.dmp", g_logDir, stamp);

		FILE* f = nullptr;
		if (_wfopen_s(&f, txtPath, L"w, ccs=UTF-8") == 0 && f)
		{
			fwprintf(f, L"The Football Life crash report\n");
			fwprintf(f, L"Time:      %s\n", stamp);
			fwprintf(f, L"Type:      %s\n", kind);
			fwprintf(f, L"Detail:    %s\n", detail ? detail : L"");
			fwprintf(f, L"Code:      0x%08X\n", code);
			fwprintf(f, L"Address:   0x%p\n", address);
			fwprintf(f, L"Context:   %s\n", g_context);
			fwprintf(f, L"Dump:      crash-%s.dmp\n\n", stamp);

			// Raw stack as module+offset - resolvable against the build's PDB.
			void* frames[32] = {};
			USHORT n = CaptureStackBackTrace(0, 32, frames, nullptr);
			fwprintf(f, L"Stack:\n");
			for (USHORT i = 0; i < n; ++i)
			{
				HMODULE mod = nullptr;
				wchar_t modPath[MAX_PATH] = {};
				if (GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
					GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
					reinterpret_cast<LPCWSTR>(frames[i]), &mod) && mod)
				{
					GetModuleFileNameW(mod, modPath, MAX_PATH);
					wchar_t const* name = wcsrchr(modPath, L'\\');
					fwprintf(f, L"  #%02u %s+0x%llX\n", i, name ? name + 1 : modPath,
						static_cast<unsigned long long>(reinterpret_cast<ULONG_PTR>(frames[i]) -
							reinterpret_cast<ULONG_PTR>(mod)));
				}
				else
				{
					fwprintf(f, L"  #%02u 0x%p\n", i, frames[i]);
				}
			}
			fclose(f);
		}

		WriteMiniDump(ep, dmpPath);
	}

	LONG WINAPI UnhandledFilter(EXCEPTION_POINTERS* ep)
	{
		WriteReport(L"Native exception (SEH)", L"", ep->ExceptionRecord->ExceptionCode,
			ep->ExceptionRecord->ExceptionAddress, ep);
		return EXCEPTION_CONTINUE_SEARCH; // let Windows terminate the process as normal
	}

	void TerminateHandler()
	{
		WriteReport(L"Uncaught C++ exception (std::terminate)", L"", 0, nullptr, nullptr);
		TerminateProcess(GetCurrentProcess(), 1);
	}
}

namespace CrashLogger
{
	void Install()
	{
		// Resolve the log folder now - don't touch WinRT or the shell inside a crash handler.
		std::wstring dir;
		bool ok = false;

		// 1) Documents\The Football Life\Logs
		PWSTR docs = nullptr;
		if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Documents, 0, nullptr, &docs)) && docs)
		{
			dir = std::wstring(docs) + L"\\The Football Life\\Logs";
			ok = TryUseLogDir(dir);
		}
		if (docs) CoTaskMemFree(docs);

		// 2) Packaged LocalState\Logs
		if (!ok)
		{
			try
			{
				auto local = winrt::Windows::Storage::ApplicationData::Current().LocalFolder().Path();
				dir = std::wstring(local.c_str()) + L"\\Logs";
				ok = TryUseLogDir(dir);
			}
			catch (...) {}
		}

		// 3) Temp folder
		if (!ok)
		{
			wchar_t temp[MAX_PATH] = {};
			GetTempPathW(MAX_PATH, temp);
			dir = std::wstring(temp) + L"TheFootballLife\\Logs";
			TryUseLogDir(dir);
		}

		wcsncpy_s(g_logDir, dir.c_str(), _TRUNCATE);

		SetUnhandledExceptionFilter(UnhandledFilter);
		std::set_terminate(TerminateHandler);
	}

	void SetContext(std::wstring const& text)
	{
		wcsncpy_s(g_context, text.c_str(), _TRUNCATE);
	}

	void LogXamlException(wchar_t const* message, unsigned int hresult)
	{
		WriteReport(L"Unhandled XAML exception", message, hresult, nullptr, nullptr);
	}

	std::wstring GetLogDirectory()
	{
		return std::wstring(g_logDir);
	}
}
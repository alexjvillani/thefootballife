#include "pch.h"
#include "XFactorService.h"

#include <fstream>
#include <sstream>
#include <cctype>
#include <Windows.h>

namespace
{
	// --- CSV helpers: deliberately mirror DayEventService.cpp's (which in
	// turn mirror CareerHubPage.xaml.cpp's private static helpers) rather
	// than sharing them, keeping this service self-contained without
	// refactoring a working file.

	std::string TrimA(std::string s)
	{
		auto l = s.find_first_not_of(" \t\r\n");
		if (l == std::string::npos) return {};
		auto r = s.find_last_not_of(" \t\r\n");
		return s.substr(l, r - l + 1);
	}

	std::wstring ToW(std::string const& s)
	{
		if (s.empty()) return {};
		int n = ::MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), nullptr, 0);
		if (n == 0) return std::wstring(s.begin(), s.end());
		std::wstring out(n, L'\0');
		::MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), &out[0], n);
		return out;
	}

	std::vector<std::string> ParseCsvLine(std::string const& line)
	{
		std::vector<std::string> out;
		std::string cur;
		bool inQ = false;
		for (size_t i = 0; i < line.size(); ++i)
		{
			char c = line[i];
			if (inQ)
			{
				if (c == '"')
				{
					if (i + 1 < line.size() && line[i + 1] == '"') { cur += '"'; ++i; }
					else inQ = false;
				}
				else cur += c;
			}
			else
			{
				if (c == '"') inQ = true;
				else if (c == ',') { out.push_back(TrimA(cur)); cur.clear(); }
				else cur += c;
			}
		}
		out.push_back(TrimA(cur));
		return out;
	}

	std::string NormHdr(std::string s)
	{
		std::string o;
		for (char c : s)
			if (std::isalnum(static_cast<unsigned char>(c)))
				o += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
		return o;
	}

	int HdrIdx(std::unordered_map<std::string, int> const& m, std::string const& name)
	{
		auto it = m.find(name);
		return it != m.end() ? it->second : -1;
	}

	void StripBom(std::string& s)
	{
		if (s.size() >= 3 &&
			static_cast<unsigned char>(s[0]) == 0xEF &&
			static_cast<unsigned char>(s[1]) == 0xBB &&
			static_cast<unsigned char>(s[2]) == 0xBF)
			s.erase(0, 3);
	}

	std::string FindCsv(std::string const& relativePath)
	{
		std::vector<std::string> candidates;

		auto tryAdd = [&](std::string base)
			{
				while (!base.empty() && (base.back() == '\\' || base.back() == '/')) base.pop_back();
				candidates.push_back(base + "\\" + relativePath);
				candidates.push_back(base + "\\..\\" + relativePath);
				candidates.push_back(base + "\\..\\..\\" + relativePath);
			};

		char buf[MAX_PATH] = {};
		if (GetCurrentDirectoryA(MAX_PATH, buf) > 0) tryAdd(buf);

		char exe[MAX_PATH] = {};
		if (GetModuleFileNameA(nullptr, exe, MAX_PATH) > 0)
		{
			std::string ep(exe);
			auto p = ep.find_last_of("\\/");
			if (p != std::string::npos) tryAdd(ep.substr(0, p));
		}

		const std::vector<std::string> roots = {
			"thefootballife", "thefootball_life", "FootballLife", "football_life"
		};
		DWORD dm = GetLogicalDrives();
		for (int i = 2; i < 26; ++i)
		{
			if (!(dm & (1u << i))) continue;
			char drive = static_cast<char>('A' + i);
			for (auto const& root : roots)
			{
				std::string base;
				base += drive;
				base += ":\\" + root;
				tryAdd(base);
			}
		}

		for (auto const& c : candidates)
		{
			std::ifstream f(c, std::ios::binary);
			if (f.is_open()) return c;
		}
		return {};
	}

	// Parses "Confidence:+5;InjuryRisk:+3" into StatDeltas entries.
	void ParseEffects(std::string const& raw, std::unordered_map<std::wstring, int>& out)
	{
		std::istringstream ss(raw);
		std::string pair;
		while (std::getline(ss, pair, ';'))
		{
			pair = TrimA(pair);
			if (pair.empty()) continue;

			auto colon = pair.find(':');
			if (colon == std::string::npos) continue;

			std::string statName = TrimA(pair.substr(0, colon));
			std::string deltaStr = TrimA(pair.substr(colon + 1));
			if (statName.empty() || deltaStr.empty()) continue;

			try
			{
				int delta = std::stoi(deltaStr);
				out[ToW(statName)] = delta;
			}
			catch (...) { /* skip malformed entries rather than fail the whole load */ }
		}
	}
}

namespace XFactorService
{
	std::vector<XFactorOption> LoadOptions(std::wstring const& category)
	{
		std::vector<XFactorOption> options;

		std::string csvPath = FindCsv("Assets\\Data\\xfactors.csv");
		if (csvPath.empty()) return options;

		std::ifstream file(csvPath, std::ios::binary);
		if (!file.is_open()) return options;

		std::string headerLine;
		if (!std::getline(file, headerLine)) return options;

		StripBom(headerLine);
		if (!headerLine.empty() && headerLine.back() == '\r') headerLine.pop_back();

		auto hparts = ParseCsvLine(headerLine);
		std::unordered_map<std::string, int> hmap;
		for (size_t i = 0; i < hparts.size(); ++i)
			hmap[NormHdr(hparts[i])] = static_cast<int>(i);

		int iCategory = HdrIdx(hmap, "category");
		int iId = HdrIdx(hmap, "id");
		int iTitle = HdrIdx(hmap, "title");
		int iDesc = HdrIdx(hmap, "description");
		int iEffects = HdrIdx(hmap, "stateffects");

		auto field = [](std::vector<std::string> const& p, int idx) -> std::string
			{
				return (idx >= 0 && idx < static_cast<int>(p.size())) ? p[idx] : std::string{};
			};

		std::string row;
		while (std::getline(file, row))
		{
			if (!row.empty() && row.back() == '\r') row.pop_back();
			if (row.empty()) continue;

			auto p = ParseCsvLine(row);

			if (ToW(field(p, iCategory)) != category) continue;

			XFactorOption opt;
			opt.Id = ToW(field(p, iId));
			opt.Title = ToW(field(p, iTitle));
			opt.Description = ToW(field(p, iDesc));
			if (opt.Id.empty() || opt.Title.empty()) continue;

			ParseEffects(field(p, iEffects), opt.StatDeltas);

			options.push_back(std::move(opt));
		}

		return options;
	}
}
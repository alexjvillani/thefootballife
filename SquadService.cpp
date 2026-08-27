#include "pch.h"
#include "SquadService.h"

#include <fstream>
#include <sstream>
#include <random>
#include <algorithm>
#include <cctype>
#include <unordered_map>
#include <Windows.h>

namespace
{
	// --- CSV helpers: deliberately mirror DayEventService.cpp's / 
	// XFactorService.cpp's (which in turn mirror CareerHubPage.xaml.cpp's
	// private static helpers) rather than sharing them, keeping this
	// service self-contained without refactoring a working file.

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

	std::mt19937& Rng()
	{
		static std::mt19937 rng{ std::random_device{}() };
		return rng;
	}

	struct NamePools
	{
		std::vector<std::wstring> FirstNames;
		std::vector<std::wstring> LastNames;
	};

	NamePools LoadNamePools()
	{
		NamePools pools;

		std::string csvPath = FindCsv("Assets\\Data\\teammatenames.csv");
		if (!csvPath.empty())
		{
			std::ifstream file(csvPath, std::ios::binary);
			if (file.is_open())
			{
				std::string headerLine;
				if (std::getline(file, headerLine))
				{
					StripBom(headerLine);
					if (!headerLine.empty() && headerLine.back() == '\r') headerLine.pop_back();

					auto hparts = ParseCsvLine(headerLine);
					std::unordered_map<std::string, int> hmap;
					for (size_t i = 0; i < hparts.size(); ++i)
						hmap[NormHdr(hparts[i])] = static_cast<int>(i);

					int iFirst = HdrIdx(hmap, "firstname");
					int iLast = HdrIdx(hmap, "lastname");

					std::string row;
					while (std::getline(file, row))
					{
						if (!row.empty() && row.back() == '\r') row.pop_back();
						if (row.empty()) continue;

						auto p = ParseCsvLine(row);

						// Columns are independent pools, not row-paired -
						// this lets the CSV hold mismatched-length lists
						// (one column can run longer than the other) and
						// still cross-combine into far more unique full
						// names than either column has rows.
						if (iFirst >= 0 && iFirst < static_cast<int>(p.size()) && !p[iFirst].empty())
						{
							pools.FirstNames.push_back(ToW(p[iFirst]));
						}
						if (iLast >= 0 && iLast < static_cast<int>(p.size()) && !p[iLast].empty())
						{
							pools.LastNames.push_back(ToW(p[iLast]));
						}
					}
				}
			}
		}

		if (pools.FirstNames.empty() || pools.LastNames.empty())
		{
			// Small built-in fallback pool so a squad can always be
			// generated even if teammatenames.csv can't be found at
			// runtime - same FindCsv path-resolution / "Copy to Output
			// Directory" risk every other CSV in this project has.
			pools.FirstNames = { L"Jack", L"Liam", L"Noah", L"Oliver", L"Ethan", L"Cooper", L"Lachlan", L"Mitchell" };
			pools.LastNames = { L"Thompson", L"Walsh", L"Fletcher", L"Hayes", L"Brennan", L"Doyle", L"Reid", L"Carter" };
		}

		return pools;
	}
}

namespace SquadService
{
	std::vector<SquadMember> GenerateSquad(int count, int minOverall, int maxOverall)
	{
		std::vector<SquadMember> squad;

		auto pools = LoadNamePools();
		if (pools.FirstNames.empty() || pools.LastNames.empty())
		{
			return squad;
		}

		std::uniform_int_distribution<int> firstDist(0, static_cast<int>(pools.FirstNames.size()) - 1);
		std::uniform_int_distribution<int> lastDist(0, static_cast<int>(pools.LastNames.size()) - 1);

		const std::vector<std::wstring> positions = { L"Forward", L"Midfielder", L"Defender", L"Ruck" };
		std::uniform_int_distribution<int> positionDist(0, static_cast<int>(positions.size()) - 1);
		std::uniform_int_distribution<int> overallDist(minOverall, maxOverall);

		std::vector<std::wstring> usedFullNames;
		int attempts = 0;
		int maxAttempts = count * 20; // generous bound - collisions are rare across a 5000+ combination pool

		while (static_cast<int>(squad.size()) < count && attempts < maxAttempts)
		{
			attempts++;

			std::wstring first = pools.FirstNames[firstDist(Rng())];
			std::wstring last = pools.LastNames[lastDist(Rng())];
			std::wstring fullName = first + L" " + last;

			if (std::find(usedFullNames.begin(), usedFullNames.end(), fullName) != usedFullNames.end())
			{
				continue; // same full name already in this roster - try again
			}
			usedFullNames.push_back(fullName);

			SquadMember member;
			member.FirstName = first;
			member.LastName = last;
			member.Position = positions[positionDist(Rng())];
			member.Overall = overallDist(Rng());
			squad.push_back(member);
		}

		return squad;
	}
}
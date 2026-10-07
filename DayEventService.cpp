#include "pch.h"
#include "DayEventService.h"

#include <algorithm>
#include <fstream>
#include <sstream>
#include <random>
#include <cctype>
#include <Windows.h>

namespace
{
	// --- CSV helpers: deliberately mirror CareerHubPage.xaml.cpp's private
	// static helpers (TrimA/ToW/ParseCsvLine/NormHdr/HdrIdx/FindCsv) rather
	// than sharing them, since those are file-local statics there and this
	// keeps DayEventService self-contained without refactoring a working file.

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

	// Parses "Fatigue:-5;Confidence:+3" into StatDeltas entries.
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
			catch (...) { /* skip malformed entries rather than fail the whole event */ }
		}
	}

	// Splits "a;b;c" into trimmed, non-empty entries.
	std::vector<std::wstring> ParseList(std::string const& raw)
	{
		std::vector<std::wstring> out;
		std::istringstream ss(raw);
		std::string item;
		while (std::getline(ss, item, ';'))
		{
			item = TrimA(item);
			if (!item.empty()) out.push_back(ToW(item));
		}
		return out;
	}

	// Parses "FamilyBond>=2;Ambition<3" into counter conditions. Supported
	// operators: >=  <=  >  <  ==  =  !=  ("=" is treated as "=="). Malformed
	// terms are skipped rather than failing the whole event.
	void ParseConditions(std::string const& raw, std::vector<DayEventService::CounterCondition>& out)
	{
		std::istringstream ss(raw);
		std::string part;
		while (std::getline(ss, part, ';'))
		{
			part = TrimA(part);
			if (part.empty()) continue;

			auto p = part.find_first_of("<>=!");
			if (p == std::string::npos || p == 0) continue;

			std::string op(1, part[p]);
			size_t valueStart = p + 1;
			if (valueStart < part.size() && part[valueStart] == '=')
			{
				op += '=';
				++valueStart;
			}
			if (op == "=") op = "==";
			if (op == "!") continue; // a lone '!' isn't an operator

			try
			{
				DayEventService::CounterCondition c;
				c.Name = ToW(TrimA(part.substr(0, p)));
				c.Op = ToW(op);
				c.Value = std::stoi(TrimA(part.substr(valueStart)));
				if (!c.Name.empty()) out.push_back(std::move(c));
			}
			catch (...) { /* skip malformed condition */ }
		}
	}

	bool CompareCounter(int lhs, std::wstring const& op, int rhs)
	{
		if (op == L">=") return lhs >= rhs;
		if (op == L"<=") return lhs <= rhs;
		if (op == L">")  return lhs > rhs;
		if (op == L"<")  return lhs < rhs;
		if (op == L"==") return lhs == rhs;
		if (op == L"!=") return lhs != rhs;
		return false;
	}

	std::mt19937& Rng()
	{
		static std::mt19937 rng{ std::random_device{}() };
		return rng;
	}
}

namespace DayEventService
{
	std::vector<DayEvent> LoadEvents()
	{
		std::vector<DayEvent> events;

		std::string csvPath = FindCsv("Assets\\Data\\events.csv");
		if (csvPath.empty()) return events;

		std::ifstream file(csvPath, std::ios::binary);
		if (!file.is_open()) return events;

		std::string headerLine;
		if (!std::getline(file, headerLine)) return events;

		StripBom(headerLine);
		if (!headerLine.empty() && headerLine.back() == '\r') headerLine.pop_back();

		auto hparts = ParseCsvLine(headerLine);
		std::unordered_map<std::string, int> hmap;
		for (size_t i = 0; i < hparts.size(); ++i)
			hmap[NormHdr(hparts[i])] = static_cast<int>(i);

		int iId = HdrIdx(hmap, "eventid");
		int iTitle = HdrIdx(hmap, "title");
		int iDesc = HdrIdx(hmap, "description");
		int iRequiresFlag = HdrIdx(hmap, "requiresflag");
		int iExcludesFlag = HdrIdx(hmap, "excludesflag");
		int iRequiresAny = HdrIdx(hmap, "requiresany");
		int iRequiresCounters = HdrIdx(hmap, "requirescounters");
		int iRepeatable = HdrIdx(hmap, "repeatable");
		int iWeight = HdrIdx(hmap, "weight");

		// Columns for each of the (up to) 3 choices. Header-name driven, so
		// the new Clear/Counters columns are optional - an older events.csv
		// without them still loads (those fields just come back empty).
		struct ChoiceCols { int Label; int Effects; int SetFlag; int ClearFlag; int Counters; };
		const ChoiceCols choiceCols[3] = {
			{ HdrIdx(hmap, "choice1label"), HdrIdx(hmap, "choice1effects"), HdrIdx(hmap, "choice1setflag"), HdrIdx(hmap, "choice1clearflag"), HdrIdx(hmap, "choice1counters") },
			{ HdrIdx(hmap, "choice2label"), HdrIdx(hmap, "choice2effects"), HdrIdx(hmap, "choice2setflag"), HdrIdx(hmap, "choice2clearflag"), HdrIdx(hmap, "choice2counters") },
			{ HdrIdx(hmap, "choice3label"), HdrIdx(hmap, "choice3effects"), HdrIdx(hmap, "choice3setflag"), HdrIdx(hmap, "choice3clearflag"), HdrIdx(hmap, "choice3counters") },
		};

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

			DayEvent ev;
			ev.EventId = ToW(field(p, iId));
			ev.Title = ToW(field(p, iTitle));
			ev.Description = ToW(field(p, iDesc));
			if (ev.EventId.empty()) continue;

			// RequiresFlag / ExcludesFlag accept "a;b" lists. A single
			// flag name works exactly as it did before lists existed.
			ev.RequiresFlags = ParseList(field(p, iRequiresFlag));
			ev.ExcludesFlags = ParseList(field(p, iExcludesFlag));
			ev.RequiresAnyFlags = ParseList(field(p, iRequiresAny));
			ParseConditions(field(p, iRequiresCounters), ev.RequiresCounters);

			// Blank/absent = repeatable (the pre-existing behaviour).
			std::string repeatable = NormHdr(field(p, iRepeatable));
			ev.Repeatable = !(repeatable == "0" || repeatable == "false" || repeatable == "no");

			// Blank, malformed or non-positive = 1 (a normal ambient event).
			try
			{
				std::string weight = field(p, iWeight);
				ev.Weight = weight.empty() ? 1 : (std::max)(1, std::stoi(weight));
			}
			catch (...) { ev.Weight = 1; }

			for (auto const& cols : choiceCols)
			{
				std::string label = field(p, cols.Label);
				if (label.empty()) continue;

				EventChoice c;
				c.Label = ToW(label);
				ParseEffects(field(p, cols.Effects), c.StatDeltas);
				c.SetFlags = ParseList(field(p, cols.SetFlag));
				c.ClearFlags = ParseList(field(p, cols.ClearFlag));
				ParseEffects(field(p, cols.Counters), c.CounterDeltas);
				ev.Choices.push_back(std::move(c));
			}

			if (ev.Choices.size() >= 2) events.push_back(std::move(ev));
			// Events with fewer than 2 choices are silently skipped -
			// malformed row, not worth crashing the whole load over.
		}

		return events;
	}

	bool IsEligible(DayEvent const& ev, NarrativeState const& narrative)
	{
		// One-shot events never come back once the player has decided them.
		if (!ev.Repeatable && narrative.HasSeen(ev.EventId)) return false;

		for (auto const& flag : ev.RequiresFlags)
		{
			if (!narrative.HasFlag(flag)) return false;
		}

		if (!ev.RequiresAnyFlags.empty())
		{
			bool any = false;
			for (auto const& flag : ev.RequiresAnyFlags)
			{
				if (narrative.HasFlag(flag)) { any = true; break; }
			}
			if (!any) return false;
		}

		for (auto const& flag : ev.ExcludesFlags)
		{
			if (narrative.HasFlag(flag)) return false;
		}

		for (auto const& cond : ev.RequiresCounters)
		{
			if (!CompareCounter(narrative.Counter(cond.Name), cond.Op, cond.Value)) return false;
		}

		return true;
	}

	DayEvent const* RollForEvent(
		std::vector<DayEvent> const& events,
		int percentChance,
		NarrativeState const& narrative)
	{
		if (events.empty()) return nullptr;

		std::vector<DayEvent const*> eligible;
		for (auto const& ev : events)
		{
			if (IsEligible(ev, narrative))
			{
				eligible.push_back(&ev);
			}
		}
		if (eligible.empty()) return nullptr;

		std::uniform_int_distribution<int> chanceRoll(1, 100);
		if (chanceRoll(Rng()) > percentChance) return nullptr;

		// Weighted pick among the eligible events.
		int totalWeight = 0;
		for (auto const* ev : eligible) totalWeight += (std::max)(1, ev->Weight);

		std::uniform_int_distribution<int> pick(1, totalWeight);
		int target = pick(Rng());
		for (auto const* ev : eligible)
		{
			target -= (std::max)(1, ev->Weight);
			if (target <= 0) return ev;
		}
		return eligible.back(); // not reachable; keeps the compiler happy
	}

	void ApplyNarrativeEffects(EventChoice const& choice, NarrativeState& narrative)
	{
		// Clear before set, so a choice that lists the same flag in both
		// ends up with it set.
		for (auto const& flag : choice.ClearFlags) narrative.Flags.erase(flag);
		for (auto const& flag : choice.SetFlags) narrative.Flags.insert(flag);

		for (auto const& [name, delta] : choice.CounterDeltas)
		{
			narrative.Counters[name] = std::clamp(narrative.Counter(name) + delta, -100, 100);
		}
	}
}
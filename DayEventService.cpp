#include "pch.h"
#include "DayEventService.h"

#include <algorithm>
#include <fstream>
#include <sstream>
#include <random>
#include <cctype>
#include <cwctype>
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

	// Parses "FamilyBond:+2;PartnerBond=0": ':' adds a delta, '=' sets an exact value.
	void ParseCounterChanges(std::string const& raw, std::unordered_map<std::wstring, int>& deltas, std::unordered_map<std::wstring, int>& sets)
	{
		std::istringstream ss(raw);
		std::string part;
		while (std::getline(ss, part, ';'))
		{
			part = TrimA(part);
			auto eq = part.find('=');
			if (eq != std::string::npos)
			{
				try { sets[ToW(TrimA(part.substr(0, eq)))] = std::stoi(TrimA(part.substr(eq + 1))); }
				catch (...) {}
			}
		}
		// Everything with ':' is handled by the existing delta parser; entries
		// with '=' have no ':' so it skips them.
		ParseEffects(raw, deltas);
	}

	// Parses "partner;sponsor=Stride" into fact conditions.
	void ParseFactConditions(std::string const& raw, std::vector<DayEventService::FactCondition>& out)
	{
		std::istringstream ss(raw);
		std::string part;
		while (std::getline(ss, part, ';'))
		{
			part = TrimA(part);
			if (part.empty()) continue;
			DayEventService::FactCondition c;
			auto eq = part.find('=');
			if (eq == std::string::npos)
			{
				c.Key = ToW(part);
			}
			else
			{
				c.Key = ToW(TrimA(part.substr(0, eq)));
				c.Value = ToW(TrimA(part.substr(eq + 1)));
				c.HasValue = true;
			}
			if (!c.Key.empty()) out.push_back(std::move(c));
		}
	}

	// Parses "partner=Mia|Chloe;mood=happy" into (key, raw value) pairs.
	void ParseFactAssignments(std::string const& raw, std::vector<std::pair<std::wstring, std::wstring>>& out)
	{
		std::istringstream ss(raw);
		std::string part;
		while (std::getline(ss, part, ';'))
		{
			part = TrimA(part);
			auto eq = part.find('=');
			if (eq == std::string::npos || eq == 0) continue;
			std::wstring key = ToW(TrimA(part.substr(0, eq)));
			std::wstring value = ToW(TrimA(part.substr(eq + 1)));
			if (!key.empty()) out.emplace_back(std::move(key), std::move(value));
		}
	}

	bool FactMatches(DayEventService::FactCondition const& c, NarrativeState const& n)
	{
		return c.HasValue ? (n.Fact(c.Key) == c.Value) : n.HasFact(c.Key);
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

	// ---- generic CSV table reading (names / jobs / epilogue) ----
	using CsvRow = std::unordered_map<std::string, std::string>;

	std::vector<CsvRow> ReadTable(std::string const& relativePath)
	{
		std::vector<CsvRow> rows;
		std::string path = FindCsv(relativePath);
		if (path.empty()) return rows;
		std::ifstream file(path, std::ios::binary);
		if (!file.is_open()) return rows;

		std::string headerLine;
		if (!std::getline(file, headerLine)) return rows;
		StripBom(headerLine);
		if (!headerLine.empty() && headerLine.back() == '\r') headerLine.pop_back();
		auto headers = ParseCsvLine(headerLine);
		for (auto& h : headers) h = NormHdr(h);

		std::string line;
		while (std::getline(file, line))
		{
			if (!line.empty() && line.back() == '\r') line.pop_back();
			if (TrimA(line).empty()) continue;
			auto parts = ParseCsvLine(line);
			CsvRow row;
			for (size_t i = 0; i < headers.size() && i < parts.size(); ++i) row[headers[i]] = parts[i];
			rows.push_back(std::move(row));
		}
		return rows;
	}

	std::string Cell(CsvRow const& row, char const* key)
	{
		auto it = row.find(key);
		return it != row.end() ? it->second : std::string();
	}

	int CellInt(CsvRow const& row, char const* key, int fallback)
	{
		try { auto s = Cell(row, key); return s.empty() ? fallback : std::stoi(s); }
		catch (...) { return fallback; }
	}

	std::wstring Lower(std::wstring s)
	{
		for (auto& c : s) c = static_cast<wchar_t>(std::towlower(c));
		return s;
	}

	// The condition checks shared by day events and epilogue lines.
	bool ConditionsMet(
		std::vector<std::wstring> const& requiresFlags,
		std::vector<std::wstring> const& requiresAnyFlags,
		std::vector<std::wstring> const& excludesFlags,
		std::vector<DayEventService::CounterCondition> const& requiresCounters,
		std::vector<DayEventService::CounterCondition> const& requiresStats,
		std::vector<DayEventService::FactCondition> const& requiresFacts,
		std::vector<DayEventService::FactCondition> const& excludesFacts,
		NarrativeState const& narrative,
		DayEventService::StatMap const& stats)
	{
		for (auto const& flag : requiresFlags)
		{
			if (!narrative.HasFlag(flag)) return false;
		}
		if (!requiresAnyFlags.empty())
		{
			bool any = false;
			for (auto const& flag : requiresAnyFlags)
			{
				if (narrative.HasFlag(flag)) { any = true; break; }
			}
			if (!any) return false;
		}
		for (auto const& flag : excludesFlags)
		{
			if (narrative.HasFlag(flag)) return false;
		}
		for (auto const& cond : requiresCounters)
		{
			if (!CompareCounter(narrative.Counter(cond.Name), cond.Op, cond.Value)) return false;
		}
		for (auto const& cond : requiresStats)
		{
			auto it = stats.find(cond.Name);
			int value = it != stats.end() ? it->second : 0;
			if (!CompareCounter(value, cond.Op, cond.Value)) return false;
		}
		for (auto const& cond : requiresFacts)
		{
			if (!FactMatches(cond, narrative)) return false;
		}
		for (auto const& cond : excludesFacts)
		{
			if (FactMatches(cond, narrative)) return false;
		}
		return true;
	}

	std::mt19937& Rng()
	{
		static std::mt19937 rng{ std::random_device{}() };
		return rng;
	}
}

namespace DayEventService
{
	// Loads one events CSV into `events`. Returns false if the file couldn't
	// be found/opened (so a missing optional file is simply skipped). Each
	// file is read by header NAME, so it only needs the columns it uses.
	// An EventId already loaded from an earlier file is ignored, so two files
	// can never silently double up an event.
	static bool LoadEventsFromFile(std::string const& relativePath, std::vector<DayEvent>& events, std::unordered_set<std::wstring>& seenIds)
	{
		std::string csvPath = FindCsv(relativePath);
		if (csvPath.empty()) return false;

		std::ifstream file(csvPath, std::ios::binary);
		if (!file.is_open()) return false;

		std::string headerLine;
		if (!std::getline(file, headerLine)) return true;

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
		int iRequiresFact = HdrIdx(hmap, "requiresfact");
		int iRequiresStats = HdrIdx(hmap, "requiresstats");
		int iExcludesFact = HdrIdx(hmap, "excludesfact");

		// Columns for each of the (up to) 3 choices. Header-name driven, so
		// the new Clear/Counters columns are optional - an older events.csv
		// without them still loads (those fields just come back empty).
		struct ChoiceCols { int Label; int Effects; int SetFlag; int ClearFlag; int Counters; int SetFact; int ClearFact; };
		const ChoiceCols choiceCols[3] = {
			{ HdrIdx(hmap, "choice1label"), HdrIdx(hmap, "choice1effects"), HdrIdx(hmap, "choice1setflag"), HdrIdx(hmap, "choice1clearflag"), HdrIdx(hmap, "choice1counters"), HdrIdx(hmap, "choice1setfact"), HdrIdx(hmap, "choice1clearfact") },
			{ HdrIdx(hmap, "choice2label"), HdrIdx(hmap, "choice2effects"), HdrIdx(hmap, "choice2setflag"), HdrIdx(hmap, "choice2clearflag"), HdrIdx(hmap, "choice2counters"), HdrIdx(hmap, "choice2setfact"), HdrIdx(hmap, "choice2clearfact") },
			{ HdrIdx(hmap, "choice3label"), HdrIdx(hmap, "choice3effects"), HdrIdx(hmap, "choice3setflag"), HdrIdx(hmap, "choice3clearflag"), HdrIdx(hmap, "choice3counters"), HdrIdx(hmap, "choice3setfact"), HdrIdx(hmap, "choice3clearfact") },
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
			ParseConditions(field(p, iRequiresStats), ev.RequiresStats);

			// Blank/absent = repeatable (the pre-existing behaviour).
			std::string repeatable = NormHdr(field(p, iRepeatable));
			if (repeatable == "0" || repeatable == "false" || repeatable == "no" || repeatable == "never")
				ev.Repeat = DayEventService::RepeatRule::Never;
			else if (repeatable == "season" || repeatable == "perseason")
				ev.Repeat = DayEventService::RepeatRule::PerSeason;

			ParseFactConditions(field(p, iRequiresFact), ev.RequiresFacts);
			ParseFactConditions(field(p, iExcludesFact), ev.ExcludesFacts);

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
				ParseCounterChanges(field(p, cols.Counters), c.CounterDeltas, c.CounterSets);
				ParseFactAssignments(field(p, cols.SetFact), c.SetFacts);
				c.ClearFacts = ParseList(field(p, cols.ClearFact));
				ev.Choices.push_back(std::move(c));
			}

			if (ev.Choices.size() >= 2 && seenIds.insert(ev.EventId).second)
			{
				events.push_back(std::move(ev));
			}
			// Events with fewer than 2 choices (or a repeated id) are
			// silently skipped - not worth crashing the whole load over.
		}

		return true;
	}

	// Event files, loaded in this order (earlier files win on a repeated
	// EventId). Missing files are skipped, so adding a name here before the
	// file exists is harmless. Each file must also be in the project and set
	// to copy to the output folder, like the other Assets\Data CSVs.
	std::vector<DayEvent> LoadEvents()
	{
		static char const* const kEventFiles[] = {
			"events_core.csv",           // everyday ambient events
			"events_arcs.csv",           // family and mentor story arcs
			"events_relationships.csv",  // partner story
			"events_school.csv",         // assignments and exams
			"events_training.csv",       // extra training and coach
			"events_sponsor_media.csv",  // sponsorship and media
			"events_work.csv",           // jobs
		};

		std::vector<DayEvent> events;
		std::unordered_set<std::wstring> seenIds;
		bool anyFileFound = false;
		for (char const* name : kEventFiles)
		{
			if (LoadEventsFromFile(std::string("Assets\\Data\\") + name, events, seenIds)) anyFileFound = true;
		}

		// Older layout: everything in a single events.csv.
		if (!anyFileFound)
		{
			LoadEventsFromFile("Assets\\Data\\events.csv", events, seenIds);
		}
		return events;
	}

	bool IsEligible(DayEvent const& ev, NarrativeState const& narrative, int currentSeason, StatMap const& stats)
	{
		if (ev.Repeat == RepeatRule::Never && narrative.HasSeen(ev.EventId)) return false;
		if (ev.Repeat == RepeatRule::PerSeason && narrative.HasSeenInSeason(ev.EventId, currentSeason)) return false;

		return ConditionsMet(
			ev.RequiresFlags, ev.RequiresAnyFlags, ev.ExcludesFlags,
			ev.RequiresCounters, ev.RequiresStats, ev.RequiresFacts, ev.ExcludesFacts,
			narrative, stats);
	}

	DayEvent const* RollForEvent(
		std::vector<DayEvent> const& events,
		int percentChance,
		NarrativeState const& narrative,
		int currentSeason,
		StatMap const& stats)
	{
		if (events.empty()) return nullptr;

		std::vector<DayEvent const*> eligible;
		for (auto const& ev : events)
		{
			if (IsEligible(ev, narrative, currentSeason, stats))
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

	void ApplyNarrativeEffects(EventChoice const& choice, NarrativeState& narrative, StatMap const& stats)
	{
		// Resolve new fact values first, against the state from before this
		// choice (so "ex_partner={partner}" sees the current partner).
		std::vector<std::pair<std::wstring, std::wstring>> newFacts;
		for (auto const& [key, raw] : choice.SetFacts)
		{
			std::wstring value = raw;
			if (value.rfind(L"@names:", 0) == 0)
			{
				// "@names:partner": a random name from names.csv.
				value = PickName(value.substr(7));
				if (value.empty()) continue; // no such list: leave the fact unset
			}
			else if (value.rfind(L"@jobs:", 0) == 0)
			{
				// "@jobs:best": a job the player qualifies for. Sets the key
				// to the job title, plus "employer" and "job_tier".
				JobDef const* job = PickJob(value.substr(6), stats);
				if (!job) continue;
				newFacts.emplace_back(key, job->Title);
				newFacts.emplace_back(L"employer", job->Employer);
				newFacts.emplace_back(L"job_tier", job->Tier);
				continue;
			}
			else if (value.find(L'{') == std::wstring::npos && value.find(L'|') != std::wstring::npos)
			{
				// "Mia|Chloe|Zoe": pick one at random.
				std::vector<std::wstring> options;
				size_t start = 0;
				while (true)
				{
					size_t bar = value.find(L'|', start);
					options.push_back(value.substr(start, bar == std::wstring::npos ? bar : bar - start));
					if (bar == std::wstring::npos) break;
					start = bar + 1;
				}
				std::uniform_int_distribution<size_t> pickOption(0, options.size() - 1);
				value = options[pickOption(Rng())];
			}
			newFacts.emplace_back(key, narrative.Format(value));
		}

		for (auto const& key : choice.ClearFacts) narrative.Facts.erase(key);
		for (auto const& [key, value] : newFacts) narrative.Facts[key] = value;

		// Clear before set, so a choice that lists the same flag in both
		// ends up with it set.
		for (auto const& flag : choice.ClearFlags) narrative.Flags.erase(flag);
		for (auto const& flag : choice.SetFlags) narrative.Flags.insert(flag);

		for (auto const& [name, value] : choice.CounterSets)
		{
			narrative.Counters[name] = std::clamp(value, -100, 100);
		}
		for (auto const& [name, delta] : choice.CounterDeltas)
		{
			narrative.Counters[name] = std::clamp(narrative.Counter(name) + delta, -100, 100);
		}
	}

	// ---- Data tables ----

	static std::unordered_map<std::wstring, std::vector<std::wstring>> const& NamesTable()
	{
		static std::unordered_map<std::wstring, std::vector<std::wstring>> table = []
			{
				std::unordered_map<std::wstring, std::vector<std::wstring>> t;
				for (auto const& row : ReadTable("Assets\\Data\\names.csv"))
				{
					std::wstring category = Lower(ToW(Cell(row, "category")));
					std::wstring name = ToW(Cell(row, "name"));
					if (!category.empty() && !name.empty()) t[category].push_back(name);
				}
				return t;
			}();
		return table;
	}

	std::wstring PickName(std::wstring const& category)
	{
		auto const& table = NamesTable();
		auto it = table.find(Lower(category));
		if (it == table.end() || it->second.empty()) return {};
		std::uniform_int_distribution<size_t> pick(0, it->second.size() - 1);
		return it->second[pick(Rng())];
	}

	static std::vector<JobDef> const& JobsTable()
	{
		static std::vector<JobDef> jobs = []
			{
				std::vector<JobDef> list;
				for (auto const& row : ReadTable("Assets\\Data\\jobs.csv"))
				{
					JobDef j;
					j.Id = ToW(Cell(row, "jobid"));
					j.Title = ToW(Cell(row, "title"));
					j.Employer = ToW(Cell(row, "employer"));
					j.Tier = Lower(ToW(Cell(row, "tier")));
					j.MinAcademics = CellInt(row, "minacademics", 0);
					j.PayPerBlock = CellInt(row, "payperblock", 6);
					j.Description = ToW(Cell(row, "description"));
					if (!j.Title.empty()) list.push_back(std::move(j));
				}
				return list;
			}();
		return jobs;
	}

	JobDef const* FindJobByTitle(std::wstring const& title)
	{
		if (title.empty()) return nullptr;
		for (auto const& j : JobsTable())
		{
			if (j.Title == title) return &j;
		}
		return nullptr;
	}

	JobDef const* PickJob(std::wstring const& tier, StatMap const& stats)
	{
		auto it = stats.find(L"Academics");
		int const academics = it != stats.end() ? it->second : 0;
		std::wstring const wanted = Lower(tier);

		std::vector<JobDef const*> eligible;
		for (auto const& j : JobsTable())
		{
			if (j.MinAcademics <= academics) eligible.push_back(&j);
		}
		if (eligible.empty()) return nullptr;

		std::wstring useTier = wanted;
		if (wanted == L"best" || wanted == L"any")
		{
			// The tier of the most demanding job they qualify for.
			JobDef const* top = eligible.front();
			for (auto const* j : eligible)
			{
				if (j->MinAcademics > top->MinAcademics) top = j;
			}
			useTier = top->Tier;
		}

		std::vector<JobDef const*> candidates;
		for (auto const* j : eligible)
		{
			if (j->Tier == useTier) candidates.push_back(j);
		}
		if (candidates.empty()) return nullptr;
		std::uniform_int_distribution<size_t> pick(0, candidates.size() - 1);
		return candidates[pick(Rng())];
	}

	struct EpilogueRow
	{
		std::wstring Group;
		std::vector<std::wstring> RequiresFlags, RequiresAnyFlags, ExcludesFlags;
		std::vector<CounterCondition> RequiresCounters, RequiresStats;
		std::vector<FactCondition> RequiresFacts, ExcludesFacts;
		std::wstring Text;
	};

	static std::vector<EpilogueRow> const& EpilogueTable()
	{
		static std::vector<EpilogueRow> rows = []
			{
				std::vector<EpilogueRow> list;
				for (auto const& row : ReadTable("Assets\\Data\\epilogue.csv"))
				{
					EpilogueRow r;
					r.Group = Lower(ToW(Cell(row, "group")));
					r.Text = ToW(Cell(row, "text"));
					if (r.Group.empty() || r.Text.empty()) continue;
					r.RequiresFlags = ParseList(Cell(row, "requiresflag"));
					r.RequiresAnyFlags = ParseList(Cell(row, "requiresany"));
					r.ExcludesFlags = ParseList(Cell(row, "excludesflag"));
					ParseConditions(Cell(row, "requirescounters"), r.RequiresCounters);
					ParseConditions(Cell(row, "requiresstats"), r.RequiresStats);
					ParseFactConditions(Cell(row, "requiresfact"), r.RequiresFacts);
					ParseFactConditions(Cell(row, "excludesfact"), r.ExcludesFacts);
					list.push_back(std::move(r));
				}
				return list;
			}();
		return rows;
	}

	std::wstring BuildEpilogue(NarrativeState const& narrative, StatMap const& stats)
	{
		auto const& rows = EpilogueTable();
		if (rows.empty()) return {};

		// Groups in order of first appearance; within a group the FIRST row
		// whose conditions hold is used, so list the most specific first.
		std::vector<std::wstring> groupOrder;
		for (auto const& r : rows)
		{
			if (std::find(groupOrder.begin(), groupOrder.end(), r.Group) == groupOrder.end()) groupOrder.push_back(r.Group);
		}

		auto pickForGroup = [&](std::wstring const& group) -> std::wstring
			{
				for (auto const& r : rows)
				{
					if (r.Group != group) continue;
					if (ConditionsMet(r.RequiresFlags, r.RequiresAnyFlags, r.ExcludesFlags, r.RequiresCounters, r.RequiresStats, r.RequiresFacts, r.ExcludesFacts, narrative, stats))
					{
						return narrative.Format(r.Text);
					}
				}
				return {};
			};

		std::wstring epilogue;
		for (auto const& group : groupOrder)
		{
			if (group == L"fallback") continue; // only used when nothing else matched
			std::wstring line = pickForGroup(group);
			if (!line.empty()) epilogue += (epilogue.empty() ? L"" : L" ") + line;
		}
		if (epilogue.empty()) epilogue = pickForGroup(L"fallback");
		return epilogue;
	}
}
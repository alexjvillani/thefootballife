#include "pch.h"
#include "DayEventService.h"

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
        int iC1Label = HdrIdx(hmap, "choice1label");
        int iC1Fx = HdrIdx(hmap, "choice1effects");
        int iC2Label = HdrIdx(hmap, "choice2label");
        int iC2Fx = HdrIdx(hmap, "choice2effects");
        int iC3Label = HdrIdx(hmap, "choice3label");
        int iC3Fx = HdrIdx(hmap, "choice3effects");

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

            std::string c1Label = field(p, iC1Label);
            std::string c2Label = field(p, iC2Label);
            std::string c3Label = field(p, iC3Label);

            if (!c1Label.empty())
            {
                EventChoice c;
                c.Label = ToW(c1Label);
                ParseEffects(field(p, iC1Fx), c.StatDeltas);
                ev.Choices.push_back(std::move(c));
            }
            if (!c2Label.empty())
            {
                EventChoice c;
                c.Label = ToW(c2Label);
                ParseEffects(field(p, iC2Fx), c.StatDeltas);
                ev.Choices.push_back(std::move(c));
            }
            if (!c3Label.empty())
            {
                EventChoice c;
                c.Label = ToW(c3Label);
                ParseEffects(field(p, iC3Fx), c.StatDeltas);
                ev.Choices.push_back(std::move(c));
            }

            if (ev.Choices.size() >= 2) events.push_back(std::move(ev));
            // Events with fewer than 2 choices are silently skipped -
            // malformed row, not worth crashing the whole load over.
        }

        return events;
    }

    DayEvent const* RollForEvent(std::vector<DayEvent> const& events, int percentChance)
    {
        if (events.empty()) return nullptr;

        std::uniform_int_distribution<int> chanceRoll(1, 100);
        if (chanceRoll(Rng()) > percentChance) return nullptr;

        std::uniform_int_distribution<size_t> pick(0, events.size() - 1);
        return &events[pick(Rng())];
    }
}
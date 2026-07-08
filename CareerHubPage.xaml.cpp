#include "pch.h"
#include "CareerHubPage.xaml.h"
#if __has_include("CareerHubPage.g.cpp")
#include "CareerHubPage.g.cpp"
#endif

#include "GameState.h"
#include "SaveGameService.h"
#include "FixtureService.h"
#include "CareerDayService.h"

#include <algorithm>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <locale>
#include <cctype>
#include <iomanip>
#include <random>

#include <Windows.h>

#include <winrt/Windows.Foundation.h>
#include <winrt/Microsoft.UI.Xaml.Media.Imaging.h>
#include <winrt/Windows.UI.Xaml.Interop.h>
#include <winrt/Windows.UI.h>
#include <winrt/Windows.UI.Text.h>

using namespace winrt;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Media::Imaging;
using namespace Windows::Foundation;


namespace
{
    static inline std::string TrimA(std::string s)
    {
        auto l = s.find_first_not_of(" \t\r\n");
        if (l == std::string::npos) return {};
        auto r = s.find_last_not_of(" \t\r\n");
        return s.substr(l, r - l + 1);
    }

    static std::wstring ToW(std::string const& s)
    {
        if (s.empty()) return {};
        int n = ::MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), nullptr, 0);
        if (n == 0) return std::wstring(s.begin(), s.end());
        std::wstring out(n, L'\0');
        ::MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), &out[0], n);
        return out;
    }

    static std::vector<std::string> ParseCsvLine(std::string const& line)
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

    static std::string NormHdr(std::string s)
    {
        std::string o;
        for (char c : s)
            if (std::isalnum(static_cast<unsigned char>(c)))
                o += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        return o;
    }

    static int HdrIdx(std::unordered_map<std::string, int> const& m,
        std::initializer_list<std::string> names)
    {
        for (auto const& n : names) { auto it = m.find(n); if (it != m.end()) return it->second; }
        return -1;
    }

    static void StripBom(std::string& s)
    {
        if (s.size() >= 3 &&
            static_cast<unsigned char>(s[0]) == 0xEF &&
            static_cast<unsigned char>(s[1]) == 0xBB &&
            static_cast<unsigned char>(s[2]) == 0xBF)
            s.erase(0, 3);
    }

    // Locate localteams.csv (same search logic as TeamAssignmentPage)
    static std::string FindCsv(std::string const& relativePath)
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
            char dl = 'A' + static_cast<char>(i);
            std::string dr; dr += dl; dr += ":\\";
            for (auto const& r : roots)
                candidates.push_back(dr + r + "\\" + relativePath);
        }

        for (auto const& p : candidates)
        {
            std::ifstream f(p, std::ios::binary);
            if (f.is_open()) return p;
        }
        return {};
    }

    static std::wstring FormatPct(double pct)
    {
        std::wostringstream ss;
        ss << std::fixed << std::setprecision(1) << pct;
        return ss.str();
    }
}

namespace winrt::thefootballife::implementation
{
    CareerHubPage::CareerHubPage()
    {
        InitializeComponent();
        m_currentWeek = GameState::CurrentWeek;
        m_lastChoice = hstring(GameState::LastChoice);
        m_teamStats = GameState::TeamStats;
        m_fixtures = GameState::Fixtures;
        LoadPlayerData();
        UpdateWeekDisplay();
        UpdateBlockUI();
        UpdateStateUI();
        LoadLadderFromCsv();
        RenderLadder();
        RenderFixtures();
    }

    hstring CareerHubPage::PageTitle() { return m_pageTitle; }
    void    CareerHubPage::PageTitle(hstring const& value) { m_pageTitle = value; }

    hstring CareerHubPage::FormatHeightFeet(int totalCm)
    {
        int rounded = static_cast<int>(totalCm / 2.54 + 0.5);
        return to_hstring(rounded / 12) + L"'" + to_hstring(rounded % 12) + L"\"";
    }

    int CareerHubPage::BlocksUsed() const
    {
        return m_trainingBlocks + m_schoolBlocks + m_workBlocks + m_socialBlocks + m_recoveryBlocks;
    }

    // ── Ladder ───────────────────────────────────────────────────────────────

    void CareerHubPage::LoadLadderFromCsv()
    {
        m_ladder.clear();

        auto const& player = GameState::CurrentPlayer;
        std::wstring playerState = player.state.empty() ? L"Victoria" : player.state;
        std::wstring playerLeague = player.originalTeamLeague;


        std::unordered_map<std::wstring, bool> leagueClubs;
        for (auto const& f : m_fixtures)
        {
            leagueClubs[f.HomeClub] = true;
            leagueClubs[f.AwayClub] = true;
        }

        // Home ground lookup from CSV
        std::unordered_map<std::wstring, std::wstring> homeGrounds;
        std::string csvPath = FindCsv("Assets\\Data\\localteams.csv");
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

                    int iClub = HdrIdx(hmap, { "clubname", "club", "name" });
                    int iGround = HdrIdx(hmap, { "homeground", "ground", "venue" });

                    std::string row;
                    while (std::getline(file, row))
                    {
                        if (!row.empty() && row.back() == '\r') row.pop_back();
                        if (row.empty()) continue;

                        auto p = ParseCsvLine(row);
                        std::wstring clubName = (iClub >= 0 && iClub < (int)p.size()) ? ToW(p[iClub]) : L"";
                        if (clubName.empty()) continue;

                        std::wstring ground = (iGround >= 0 && iGround < (int)p.size()) ? ToW(p[iGround]) : L"";
                        homeGrounds[clubName] = ground;
                    }
                }
            }
        }

        if (!leagueClubs.empty())
        {
            // Normal path: build the ladder strictly from fixture participants
            for (auto const& kv : leagueClubs)
            {
                LadderEntry e;
                e.clubName = kv.first;

                auto groundIt = homeGrounds.find(e.clubName);
                if (groundIt != homeGrounds.end()) e.homeGround = groundIt->second;

                auto statsIt = m_teamStats.find(e.clubName);
                if (statsIt != m_teamStats.end())
                {
                    e.wins = statsIt->second.wins;
                    e.losses = statsIt->second.losses;
                    e.draws = statsIt->second.draws;
                    e.pointsFor = statsIt->second.pointsFor;
                    e.pointsAgainst = statsIt->second.pointsAgainst;
                }

                m_ladder.push_back(e);
            }
        }
        else
        {
            // Fallback for saves with no fixtures yet
            // fall back to the old state/league CSV filter so the ladder isn't empty.
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

                        int iState = HdrIdx(hmap, { "state" });
                        int iLeague = HdrIdx(hmap, { "league" });
                        int iClub = HdrIdx(hmap, { "clubname", "club", "name" });
                        int iGround = HdrIdx(hmap, { "homeground", "ground", "venue" });

                        std::string row;
                        while (std::getline(file, row))
                        {
                            if (!row.empty() && row.back() == '\r') row.pop_back();
                            if (row.empty()) continue;

                            auto p = ParseCsvLine(row);

                            std::wstring rowState = (iState >= 0 && iState < (int)p.size()) ? ToW(p[iState]) : L"";
                            std::wstring rowLeague = (iLeague >= 0 && iLeague < (int)p.size()) ? ToW(p[iLeague]) : L"";

                            if (rowState != playerState) continue;
                            if (!playerLeague.empty() && !rowLeague.empty() && rowLeague != playerLeague) continue;

                            LadderEntry e;
                            if (iClub >= 0 && iClub < (int)p.size()) e.clubName = ToW(p[iClub]);
                            if (iGround >= 0 && iGround < (int)p.size()) e.homeGround = ToW(p[iGround]);

                            auto it = m_teamStats.find(e.clubName);
                            if (it != m_teamStats.end())
                            {
                                e.wins = it->second.wins;
                                e.losses = it->second.losses;
                                e.draws = it->second.draws;
                                e.pointsFor = it->second.pointsFor;
                                e.pointsAgainst = it->second.pointsAgainst;
                            }

                            if (!e.clubName.empty())
                                m_ladder.push_back(e);
                        }
                    }
                }
            }
        }

        std::sort(m_ladder.begin(), m_ladder.end(), [](LadderEntry const& a, LadderEntry const& b)
            {
                if (a.ladderPoints() != b.ladderPoints()) return a.ladderPoints() > b.ladderPoints();
                return a.percentage() > b.percentage();
            });

        std::wstring title = playerLeague.empty() ? playerState + L" Ladder" : playerLeague + L" Ladder";
        LadderTitleText().Text(hstring(title));
    }

    void CareerHubPage::RenderLadder()
    {
        LadderRowsPanel().Children().Clear();

        if (m_ladder.empty())
        {
            TextBlock empty;
            empty.Text(L"No ladder data available.");
            empty.Foreground(SolidColorBrush(winrt::Windows::UI::ColorHelper::FromArgb(255, 150, 150, 150)));
            empty.FontSize(13);
            LadderRowsPanel().Children().Append(empty);
            return;
        }

        const std::wstring playerClub = GameState::CurrentPlayer.originalTeam;

        for (int pos = 0; pos < static_cast<int>(m_ladder.size()); ++pos)
        {
            auto const& e = m_ladder[pos];
            bool isPlayer = (e.clubName == playerClub);

            Border row;
            // Highlight the player's club
            auto bgColour = isPlayer
                ? winrt::Windows::UI::ColorHelper::FromArgb(255, 30, 58, 95)   // blue tint
                : winrt::Windows::UI::ColorHelper::FromArgb(255, 30, 30, 30);
            row.Background(SolidColorBrush(bgColour));

            Microsoft::UI::Xaml::CornerRadius cr{};
            cr.TopLeft = cr.TopRight = cr.BottomRight = cr.BottomLeft = 6;
            row.CornerRadius(cr);
            row.Padding(Thickness{ 6, 5, 6, 5 });

            Grid g;
            GridLength star{ 1.0, Microsoft::UI::Xaml::GridUnitType::Star };
            GridLength px22{ 22,  Microsoft::UI::Xaml::GridUnitType::Pixel };
            GridLength px26{ 26,  Microsoft::UI::Xaml::GridUnitType::Pixel };
            GridLength px48{ 48,  Microsoft::UI::Xaml::GridUnitType::Pixel };
            GridLength px32{ 32,  Microsoft::UI::Xaml::GridUnitType::Pixel };

            ColumnDefinition c0; c0.Width(px22); g.ColumnDefinitions().Append(c0);
            ColumnDefinition c1; c1.Width(star);  g.ColumnDefinitions().Append(c1);
            ColumnDefinition c2; c2.Width(px26); g.ColumnDefinitions().Append(c2);
            ColumnDefinition c3; c3.Width(px26); g.ColumnDefinitions().Append(c3);
            ColumnDefinition c4; c4.Width(px26); g.ColumnDefinitions().Append(c4);
            ColumnDefinition c5; c5.Width(px26); g.ColumnDefinitions().Append(c5);
            ColumnDefinition c6; c6.Width(px48); g.ColumnDefinitions().Append(c6);
            ColumnDefinition c7; c7.Width(px32); g.ColumnDefinitions().Append(c7);

            auto rowColour = isPlayer
                ? winrt::Windows::UI::Colors::White()
                : winrt::Windows::UI::ColorHelper::FromArgb(255, 220, 220, 220);

            auto makeCell = [&](int col, std::wstring const& text, bool bold = false)
                {
                    TextBlock tb;
                    tb.Text(hstring(text));
                    tb.FontSize(12);
                    tb.Foreground(SolidColorBrush(rowColour));
                    tb.VerticalAlignment(VerticalAlignment::Center);
                    tb.TextTrimming(TextTrimming::CharacterEllipsis);
                    if (bold) tb.FontWeight(winrt::Windows::UI::Text::FontWeights::SemiBold());
                    Grid::SetColumn(tb, col);
                    g.Children().Append(tb);
                };

            makeCell(0, std::to_wstring(pos + 1));
            makeCell(1, e.clubName, isPlayer);
            makeCell(2, std::to_wstring(e.played()));
            makeCell(3, std::to_wstring(e.wins));
            makeCell(4, std::to_wstring(e.losses));
            makeCell(5, std::to_wstring(e.draws));
            makeCell(6, FormatPct(e.percentage()));
            makeCell(7, std::to_wstring(e.ladderPoints()), true);

            row.Child(g);
            LadderRowsPanel().Children().Append(row);
        }
    }

    // ── Fixtures ─────────────────────────────────────────────────────────────

    void CareerHubPage::RenderFixtures()
    {
        FixtureRowsPanel().Children().Clear();

        const std::wstring playerClub = GameState::CurrentPlayer.originalTeam;

        std::vector<FixtureService::Fixture> myFixtures;
        for (auto const& f : m_fixtures)
            if (f.HomeClub == playerClub || f.AwayClub == playerClub)
                myFixtures.push_back(f);

        std::sort(myFixtures.begin(), myFixtures.end(),
            [](FixtureService::Fixture const& a, FixtureService::Fixture const& b)
            {
                return a.Round < b.Round;
            });

        if (myFixtures.empty())
        {
            TextBlock empty;
            empty.Text(L"No fixtures available.");
            empty.Foreground(SolidColorBrush(winrt::Windows::UI::ColorHelper::FromArgb(255, 150, 150, 150)));
            empty.FontSize(13);
            FixtureRowsPanel().Children().Append(empty);
            return;
        }

        for (auto const& f : myFixtures)
        {
            bool isHome = (f.HomeClub == playerClub);
            std::wstring opponent = isHome ? f.AwayClub : f.HomeClub;

            Border row;
            auto bgColour = (f.Round == m_currentWeek)
                ? winrt::Windows::UI::ColorHelper::FromArgb(255, 30, 58, 95)   // highlight this week
                : winrt::Windows::UI::ColorHelper::FromArgb(255, 30, 30, 30);
            row.Background(SolidColorBrush(bgColour));

            Microsoft::UI::Xaml::CornerRadius cr{};
            cr.TopLeft = cr.TopRight = cr.BottomRight = cr.BottomLeft = 6;
            row.CornerRadius(cr);
            row.Padding(Thickness{ 8, 6, 8, 6 });

            StackPanel content;
            content.Spacing(2);

            TextBlock roundLine;
            std::wstring label = L"Round " + std::to_wstring(f.Round) + L"  \u2022  " +
                (isHome ? L"vs " : L"@ ") + opponent;
            roundLine.Text(hstring(label));
            roundLine.Foreground(SolidColorBrush(winrt::Windows::UI::Colors::White()));
            roundLine.FontSize(13);
            roundLine.FontWeight(winrt::Windows::UI::Text::FontWeights::SemiBold());

            TextBlock resultLine;
            std::wstring resultText = f.Played
                ? (L"Result: " +
                    std::to_wstring(isHome ? f.HomeScore : f.AwayScore) + L" - " +
                    std::to_wstring(isHome ? f.AwayScore : f.HomeScore))
                : L"Not yet played";
            resultLine.Text(hstring(resultText));
            resultLine.Foreground(SolidColorBrush(winrt::Windows::UI::ColorHelper::FromArgb(255, 190, 190, 190)));
            resultLine.FontSize(12);

            content.Children().Append(roundLine);
            content.Children().Append(resultLine);
            row.Child(content);
            FixtureRowsPanel().Children().Append(row);
        }
    }

    void CareerHubPage::LadderViewButton_Click(IInspectable const&, RoutedEventArgs const&)
    {
        LadderContentPanel().Visibility(Visibility::Visible);
        FixturesContentPanel().Visibility(Visibility::Collapsed);
    }

    void CareerHubPage::FixturesViewButton_Click(IInspectable const&, RoutedEventArgs const&)
    {
        LadderContentPanel().Visibility(Visibility::Collapsed);
        FixturesContentPanel().Visibility(Visibility::Visible);
    }

    // ── Block UI ─────────────────────────────────────────────────────────────

    void CareerHubPage::UpdateBlockUI()
    {
        TrainingBlocksText().Text(to_hstring(m_trainingBlocks));
        SchoolBlocksText().Text(to_hstring(m_schoolBlocks));
        WorkBlocksText().Text(to_hstring(m_workBlocks));
        SocialBlocksText().Text(to_hstring(m_socialBlocks));
        RecoveryBlocksText().Text(to_hstring(m_recoveryBlocks));

        int used = BlocksUsed();
        int remaining = kTotalBlocks - used;
        BlockSummaryText().Text(to_hstring(used) + L"/" + to_hstring(kTotalBlocks) + L" blocks allocated");

        if (remaining == 0)
            BlockWarningText().Text(L"Allocation is valid. You can advance the week.");
        else if (remaining > 0)
            BlockWarningText().Text(L"Unassigned blocks: " + to_hstring(remaining) + L". Assign all blocks before advancing.");
        else
            BlockWarningText().Text(L"Over allocated by " + to_hstring(-remaining) + L". Remove blocks to continue.");
    }

    void CareerHubPage::UpdateStateUI()
    {
        PhysicalStateText().Text(
            L"Fatigue: " + to_hstring(m_fatigue) +
            L" | Injury Risk: " + to_hstring(m_injuryRisk) +
            L" | Recovery Quality: " + to_hstring(m_recoveryQuality));

        MentalStateText().Text(
            L"Confidence: " + to_hstring(m_confidence) +
            L" | Stress: " + to_hstring(m_stress) +
            L" | Motivation: " + to_hstring(m_motivation));

        LifeStateText().Text(
            L"Discipline: " + to_hstring(m_discipline) +
            L" | Finances: " + to_hstring(m_finances) +
            L" | Relationships: " + to_hstring(m_relationships));
    }

    void CareerHubPage::AdjustBlockByTag(hstring const& tag, int delta)
    {
        int* target = nullptr;
        if (tag == L"Training") target = &m_trainingBlocks;
        else if (tag == L"School")   target = &m_schoolBlocks;
        else if (tag == L"Work")     target = &m_workBlocks;
        else if (tag == L"Social")   target = &m_socialBlocks;
        else if (tag == L"Recovery") target = &m_recoveryBlocks;
        if (!target) return;

        int proposed = *target + delta;
        if (proposed < 0) { BottomHintText().Text(L"Blocks cannot go below zero."); return; }
        if (delta > 0 && BlocksUsed() + delta > kTotalBlocks)
        {
            BottomHintText().Text(L"You only have 14 total blocks each week."); return;
        }

        // Friday has no daily cap - it's the catch-up day for whatever's left of the 14.
        bool isFriday = (GameState::CurrentDay == DayPhase::Friday);
        if (!isFriday && delta > 0 && m_blocksSpentToday + delta > kBlocksPerDay)
        {
            BottomHintText().Text(L"You can only spend " + to_hstring(kBlocksPerDay) + L" blocks today. Advance to the next day for more.");
            return;
        }

        *target = proposed;
        m_blocksSpentToday = std::clamp(m_blocksSpentToday + delta, 0, kBlocksPerDay);
        BottomHintText().Text(L"Weekly schedule updated.");
        UpdateBlockUI();
    }

    void CareerHubPage::ApplyWeekSimulation()
    {
        m_fatigue = std::clamp(m_fatigue + (m_trainingBlocks * 4) + (m_workBlocks * 3) - (m_recoveryBlocks * 8), 0, 100);
        m_injuryRisk = std::clamp(m_injuryRisk + (m_fatigue / 12) + (m_trainingBlocks * 2) - (m_recoveryBlocks * 5), 0, 100);
        m_recoveryQuality = std::clamp(m_recoveryQuality + (m_recoveryBlocks * 7) - (m_workBlocks * 2), 0, 100);
        m_confidence = std::clamp(m_confidence + (m_trainingBlocks * 2) + m_socialBlocks - (m_stress / 18), 0, 100);
        m_stress = std::clamp(m_stress + (m_schoolBlocks * 2) + (m_workBlocks * 3) - (m_recoveryBlocks * 4), 0, 100);
        m_motivation = std::clamp(m_motivation + m_trainingBlocks + m_socialBlocks - (m_fatigue / 20), 0, 100);
        m_discipline = std::clamp(m_discipline + (m_schoolBlocks * 2) + m_trainingBlocks - (m_socialBlocks * 2), 0, 100);
        m_finances = std::clamp(m_finances + (m_workBlocks * 6) - m_recoveryBlocks, 0, 100);
        m_relationships = std::clamp(m_relationships + (m_socialBlocks * 4) - m_workBlocks, 0, 100);

        std::wstring consequence;
        if (m_recoveryBlocks == 0)
            consequence += L"Lack of sleep lowered performance readiness and increased injury risk. ";
        if (m_trainingBlocks >= 6)
            consequence += L"Extra training boosted stats but pushed up fatigue. ";
        if (m_workBlocks >= 4)
            consequence += L"Heavy work schedule improved finances while reducing recovery quality. ";
        if (m_socialBlocks >= 4)
        {
            consequence += L"Social time improved morale and relationships, but discipline dipped. ";
            m_discipline = std::clamp(m_discipline - 4, 0, 100);
        }
        if (consequence.empty())
            consequence = L"Balanced week. No major penalties triggered.";

        ConsequenceText().Text(hstring(consequence));

        m_lastChoice = L"Week simulated with blocks T:" + to_hstring(m_trainingBlocks) +
            L" S:" + to_hstring(m_schoolBlocks) +
            L" W:" + to_hstring(m_workBlocks) +
            L" So:" + to_hstring(m_socialBlocks) +
            L" R:" + to_hstring(m_recoveryBlocks);

        GameState::LastChoice = m_lastChoice.c_str();
        UpdateStateUI();
    }

    void CareerHubPage::SimulateWeekMatches()
    {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<int> scoreDist(40, 130);

        for (auto& f : m_fixtures)
        {
            if (f.Round != m_currentWeek || f.Played) continue;

            int homeScore = scoreDist(gen);
            int awayScore = scoreDist(gen);

            f.HomeScore = homeScore;
            f.AwayScore = awayScore;
            f.Played = true;

            auto& homeStats = m_teamStats[f.HomeClub];
            auto& awayStats = m_teamStats[f.AwayClub];

            homeStats.pointsFor += homeScore;
            homeStats.pointsAgainst += awayScore;
            awayStats.pointsFor += awayScore;
            awayStats.pointsAgainst += homeScore;

            if (homeScore > awayScore) { homeStats.wins++; awayStats.losses++; }
            else if (awayScore > homeScore) { awayStats.wins++; homeStats.losses++; }
            else { homeStats.draws++; awayStats.draws++; }
        }

        GameState::Fixtures = m_fixtures;
        GameState::TeamStats = m_teamStats;
    }

    void CareerHubPage::LoadPlayerData()
    {
        auto const& player = GameState::CurrentPlayer;

        if (!player.profileImagePath.empty())
        {
            BitmapImage bitmap;
            bitmap.UriSource(Uri(player.profileImagePath));
            ProfileImage().Source(bitmap);
        }

        PlayerNameText().Text(hstring(player.firstName + L" " + player.lastName));

        std::wstring teamLine = player.position + L" | " + player.foot + L" Foot | #" + player.number;
        if (!player.originalTeam.empty())
            teamLine += L" | " + player.originalTeam;
        if (!player.originalTeamLeague.empty())
            teamLine += L" (" + player.originalTeamLeague + L")";
        PlayerInfoText().Text(hstring(teamLine));

        HeightText().Text(L"Height: " + FormatHeightFeet(player.heightCm));

        MentalityText().Text(player.mentalityXFactor.empty()
            ? L"Mentality: None selected"
            : L"Mentality: " + hstring(player.mentalityXFactor));

        PhysicalText().Text(player.physicalXFactor.empty()
            ? L"Physical: None selected"
            : L"Physical: " + hstring(player.physicalXFactor));

        WeaknessesText().Text(player.weaknesses.empty()
            ? L"Weaknesses: None selected"
            : L"Weaknesses: " + hstring(player.weaknesses));
    }

    void CareerHubPage::UpdateWeekDisplay()
    {
        WeekText().Text(L"Week " + to_hstring(m_currentWeek) + L" - " + hstring(CareerDayService::GetTodayLabel()));
        LastChoiceText().Text(m_lastChoice);

        if (m_currentWeek <= 4)
        {
            StatusText().Text(L"Status: Local League Prospect");
            SeasonText().Text(L"Season Phase: School Season");
            WeeklyOutlookText().Text(L"A fresh week ahead. Focus on balancing development, performance, and life outside footy.");
            DevelopmentText().Text(L"Training form is steady. Recruiters have not yet locked onto your progress.");
        }
        else
        {
            StatusText().Text(L"Status: Emerging Prospect");
            SeasonText().Text(L"Season Phase: Mid-Season Push");
            WeeklyOutlookText().Text(L"Momentum is building. Your weekly choices are shaping both your football ceiling and off-field life.");
            DevelopmentText().Text(L"Coaches now track your consistency, discipline, and resilience week-to-week.");
        }
    }

    void CareerHubPage::TrainButton_Click(IInspectable const&, RoutedEventArgs const&)
    {
        m_lastChoice = L"Training selected. You focused on improving your game this week.";
        GameState::LastChoice = m_lastChoice.c_str();
        BottomHintText().Text(L"Training can improve long-term growth and sharpen performance.");
        UpdateWeekDisplay();
    }

    void CareerHubPage::PlayMatchButton_Click(IInspectable const&, RoutedEventArgs const&)
    {
        m_lastChoice = L"Play Match selected. This week will revolve around match performance.";
        GameState::LastChoice = m_lastChoice.c_str();
        BottomHintText().Text(L"Strong performances can lift confidence, selection chances, and recruiter interest.");
        UpdateWeekDisplay();
    }

    void CareerHubPage::RestButton_Click(IInspectable const&, RoutedEventArgs const&)
    {
        m_lastChoice = L"Rest / Social selected. You chose recovery and life balance this week.";
        GameState::LastChoice = m_lastChoice.c_str();
        BottomHintText().Text(L"Rest can help fatigue and mindset, but too much can slow development.");
        UpdateWeekDisplay();
    }

    void CareerHubPage::StudyButton_Click(IInspectable const&, RoutedEventArgs const&)
    {
        m_lastChoice = L"Study selected. You put time into school and off-field stability.";
        GameState::LastChoice = m_lastChoice.c_str();
        BottomHintText().Text(L"Balancing study can affect stress, discipline, and future pathways.");
        UpdateWeekDisplay();
    }

    void CareerHubPage::IncrementBlockButton_Click(IInspectable const& sender, RoutedEventArgs const&)
    {
        auto button = sender.try_as<Button>();
        if (!button) return;
        AdjustBlockByTag(unbox_value_or<hstring>(button.Tag(), L""), 1);
    }

    void CareerHubPage::DecrementBlockButton_Click(IInspectable const& sender, RoutedEventArgs const&)
    {
        auto button = sender.try_as<Button>();
        if (!button) return;
        AdjustBlockByTag(unbox_value_or<hstring>(button.Tag(), L""), -1);
    }

    void CareerHubPage::AdvanceWeekButton_Click(IInspectable const&, RoutedEventArgs const&)
    {
        // Friday is the hard gate: whatever's left of the 14 blocks must be
        // spent today, since there's no catch-up day after Saturday's match.
        if (GameState::CurrentDay == DayPhase::Friday && BlocksUsed() != kTotalBlocks)
        {
            BottomHintText().Text(L"You must allocate all 14 blocks for the week before Saturday.");
            return;
        }

        bool isMatchday = CareerDayService::AdvanceDay();
        m_currentWeek = GameState::CurrentWeek;
        m_blocksSpentToday = 0; // fresh daily cap for the new day

        if (isMatchday)
        {
            ApplyWeekSimulation();
            SimulateWeekMatches();
            BottomHintText().Text(L"Matchday! Simulation outcomes applied to your player state.");
            LoadLadderFromCsv();
            RenderLadder();
            RenderFixtures();
        }
        else if (GameState::CurrentDay == DayPhase::Sunday)
        {
            // Free recovery day - no player choice, just a passive nudge.
            m_recoveryQuality = std::clamp(m_recoveryQuality + 5, 0, 100);
            m_fatigue = std::clamp(m_fatigue - 5, 0, 100);
            UpdateStateUI();

            // Reset next week's block allocation - the player builds it up
            // fresh across the coming Mon-Fri rather than starting pre-filled.
            m_trainingBlocks = 0;
            m_schoolBlocks = 0;
            m_workBlocks = 0;
            m_socialBlocks = 0;
            m_recoveryBlocks = 0;
            UpdateBlockUI();

            BottomHintText().Text(L"Sunday - a free recovery day. Fatigue eased, ready for the week ahead.");
        }
        else
        {
            BottomHintText().Text(L"Day advanced.");
        }

        UpdateWeekDisplay();
    }

    void CareerHubPage::SaveGameButton_Click(IInspectable const&, RoutedEventArgs const&)
    {
        ComboBox slotComboBox;
        for (int slot = 1; slot <= SaveGameService::MaxSaveSlots; ++slot)
        {
            ComboBoxItem item;
            std::wstring label = L"Slot " + std::to_wstring(slot);
            label += SaveGameService::SlotExists(slot) ? L" (Overwrite)" : L" (Empty)";
            item.Content(box_value(hstring(label)));
            slotComboBox.Items().Append(item);
        }

        slotComboBox.SelectedIndex(SaveGameService::FindFirstAvailableSlot() - 1);

        ContentDialog dlg;
        dlg.Title(box_value(L"Choose Save Slot"));
        dlg.Content(slotComboBox);
        dlg.PrimaryButtonText(L"Save");
        dlg.CloseButtonText(L"Cancel");
        dlg.XamlRoot(this->XamlRoot());

        auto weakThis = get_weak();
        dlg.ShowAsync().Completed(
            [weakThis, slotComboBox](auto const& op, auto const&)
            {
                if (auto self = weakThis.get())
                {
                    if (op.GetResults() != ContentDialogResult::Primary) return;

                    int slot = static_cast<int>(slotComboBox.SelectedIndex()) + 1;
                    bool saved = SaveGameService::SaveToSlot(
                        slot, GameState::CurrentPlayer,
                        self->m_currentWeek, self->m_lastChoice.c_str(),
                        self->m_teamStats,
                        self->m_fixtures
                    );

                    ContentDialog result;
                    result.XamlRoot(self->XamlRoot());
                    result.CloseButtonText(L"OK");
                    if (saved)
                    {
                        result.Title(box_value(L"Game Saved"));
                        result.Content(box_value(L"Career saved to slot " + to_hstring(slot) + L"."));
                    }
                    else
                    {
                        result.Title(box_value(L"Save Failed"));
                        result.Content(box_value(L"Could not write the selected save slot."));
                    }
                    result.ShowAsync();
                }
            });
    }

    void CareerHubPage::ExitToMyCareerButton_Click(IInspectable const&, RoutedEventArgs const&)
    {
        Frame().Navigate(winrt::Windows::UI::Xaml::Interop::TypeName{
            L"thefootballife.MyCareerPage",
            winrt::Windows::UI::Xaml::Interop::TypeKind::Custom });
    }

    void CareerHubPage::ExitToMainMenuButton_Click(IInspectable const&, RoutedEventArgs const&)
    {
        Frame().Navigate(winrt::Windows::UI::Xaml::Interop::TypeName{
            L"thefootballife.MainMenuPage",
            winrt::Windows::UI::Xaml::Interop::TypeKind::Custom });
    }
}
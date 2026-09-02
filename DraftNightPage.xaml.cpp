#include "pch.h"
#include "DraftNightPage.xaml.h"
#include "GameState.h"
#include "FixtureService.h"
#include "CareerDayService.h"
#include "SaveGameService.h"
#if __has_include("DraftNightPage.g.cpp")
#include "DraftNightPage.g.cpp"
#include <winrt/Windows.UI.Xaml.Interop.h>
#endif

#include <fstream>
#include <sstream>
#include <random>
#include <algorithm>
#include <cctype>
#include <unordered_map>
#include <Windows.h>

using namespace winrt;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;

namespace
{
	// --- CSV helpers: mirror TeamAssignmentPage.xaml.cpp's private static

	std::string TrimAscii(std::string s)
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
				else if (c == ',') { out.push_back(TrimAscii(cur)); cur.clear(); }
				else cur += c;
			}
		}
		out.push_back(TrimAscii(cur));
		return out;
	}

	std::string NormalizeHeader(std::string s)
	{
		std::string out;
		for (char c : s)
			if (std::isalnum(static_cast<unsigned char>(c)))
				out += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
		return out;
	}

	int FindHeaderIndex(std::unordered_map<std::string, int> const& m, std::initializer_list<std::string> names)
	{
		for (auto const& n : names)
		{
			auto it = m.find(n);
			if (it != m.end()) return it->second;
		}
		return -1;
	}

	void TrimUtf8Bom(std::string& s)
	{
		if (s.size() >= 3 &&
			static_cast<unsigned char>(s[0]) == 0xEF &&
			static_cast<unsigned char>(s[1]) == 0xBB &&
			static_cast<unsigned char>(s[2]) == 0xBF)
			s.erase(0, 3);
	}

	bool FileExists(std::string const& path)
	{
		if (path.empty()) return false;
		std::ifstream f(path, std::ios::binary);
		return f.is_open();
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
			if (FileExists(c)) return c;
		}
		return {};
	}

	// Duplicates CareerHubPage's DetermineTier league-name matching (same
	// known limitation noted there: best-effort string matching against a
	// small known list, since there's no explicit tier field on PlayerData
	// yet). Kept separate rather than shared since the two pages don't
	// otherwise share a header.
	enum class Tier { Local, TalentLeague, StateLeague, Afl };

	Tier DetermineCurrentTier(std::wstring const& league)
	{
		if (league == L"AFL") return Tier::Afl;
		if (league == L"VFL" || league == L"SANFL" || league == L"WAFL") return Tier::StateLeague;
		if (league == L"Talent League" || league == L"NAB League") return Tier::TalentLeague;
		return Tier::Local;
	}

	// Which CSV holds the NEXT tier up's clubs, given the current tier.
	std::string TargetTierCsvPath(Tier currentTier)
	{
		switch (currentTier)
		{
		case Tier::Local:        return "Assets\\Data\\talentleagueteams.csv";
		case Tier::TalentLeague: return "Assets\\Data\\stateteams.csv";
		case Tier::StateLeague:  return "Assets\\Data\\aflteams.csv";
		case Tier::Afl:          return {}; // no further promotion - IsEligibleForPromotion already guards this
		}
		return {};
	}
}

namespace winrt::thefootballife::implementation
{
	DraftNightPage::DraftNightPage()
	{
		InitializeComponent();
		m_targetClubs = LoadTargetTierClubs();
	}

	hstring DraftNightPage::PageTitle() { return m_pageTitle; }
	void DraftNightPage::PageTitle(hstring const& value) { m_pageTitle = value; }

	std::vector<DraftNightPage::ClubProfile> DraftNightPage::LoadTargetTierClubs() const
	{
		std::vector<ClubProfile> clubs;

		auto tier = DetermineCurrentTier(GameState::CurrentPlayer.currentLeague);
		std::string relativePath = TargetTierCsvPath(tier);
		if (relativePath.empty()) return clubs;

		std::string csvPath = FindCsv(relativePath);
		if (csvPath.empty()) return clubs;

		std::ifstream file(csvPath, std::ios::binary);
		if (!file.is_open()) return clubs;

		std::string headerLine;
		if (!std::getline(file, headerLine)) return clubs;
		TrimUtf8Bom(headerLine);
		if (!headerLine.empty() && headerLine.back() == '\r') headerLine.pop_back();

		auto headerParts = ParseCsvLine(headerLine);
		std::unordered_map<std::string, int> headerMap;
		for (size_t i = 0; i < headerParts.size(); ++i)
			headerMap[NormalizeHeader(headerParts[i])] = static_cast<int>(i);

		int idxState = FindHeaderIndex(headerMap, { "state" });
		int idxLeague = FindHeaderIndex(headerMap, { "league" });
		int idxClub = FindHeaderIndex(headerMap, { "clubname", "club", "name" });
		int idxSuburb = FindHeaderIndex(headerMap, { "suburb", "location", "town" });
		int idxPrimary = FindHeaderIndex(headerMap, { "primary", "primarycolour", "primarycolor" });
		int idxSecondary = FindHeaderIndex(headerMap, { "secondary", "secondarycolour", "secondarycolor" });
		int idxHomeGround = FindHeaderIndex(headerMap, { "homeground", "ground", "venue" });
		int idxReputation = FindHeaderIndex(headerMap, { "reputation", "rep" });

		std::wstring playerState = GameState::CurrentPlayer.state.empty() ? L"Victoria" : GameState::CurrentPlayer.state;

		std::vector<ClubProfile> matchedByState;
		std::vector<ClubProfile> allClubs;

		std::string row;
		while (std::getline(file, row))
		{
			if (!row.empty() && row.back() == '\r') row.pop_back();
			if (row.empty()) continue;
			TrimUtf8Bom(row);

			auto parts = ParseCsvLine(row);
			auto field = [&](int idx) -> std::string
				{
					return (idx >= 0 && idx < static_cast<int>(parts.size())) ? parts[idx] : std::string{};
				};

			ClubProfile club;
			club.name = ToW(field(idxClub));
			if (club.name.empty()) continue;

			club.suburb = ToW(field(idxSuburb));
			club.primaryColour = ToW(field(idxPrimary));
			club.secondaryColour = ToW(field(idxSecondary));
			club.homeGround = ToW(field(idxHomeGround));
			club.league = ToW(field(idxLeague));

			try { club.reputation = field(idxReputation).empty() ? 50 : std::stoi(field(idxReputation)); }
			catch (...) { club.reputation = 50; }

			allClubs.push_back(club);

			std::wstring rowState = ToW(field(idxState));
			if (rowState == playerState)
			{
				matchedByState.push_back(club);
			}
		}

		clubs = !matchedByState.empty() ? matchedByState : allClubs;
		return clubs;
	}

	void DraftNightPage::RevealButton_Click(IInspectable const&, RoutedEventArgs const&)
	{
		if (m_targetClubs.empty())
		{
			RevealedClubNameText().Text(L"No clubs found for the next tier.");
			RevealedClubLeagueText().Text(L"Check the relevant teams CSV is present and copied to the output directory.");
			RevealedClubSuburbText().Text(L"");
			RevealedClubColoursText().Text(L"");
			RevealedClubGroundText().Text(L"");
			PreRevealPanel().Visibility(Visibility::Collapsed);
			RevealedPanel().Visibility(Visibility::Visible);
			m_hasRevealed = false; // Continue would have nowhere sensible to send the player - keep it unavailable
			return;
		}

		std::random_device rd;
		std::mt19937 gen(rd());
		std::uniform_int_distribution<int> pick(0, static_cast<int>(m_targetClubs.size()) - 1);
		m_revealedClub = m_targetClubs[pick(gen)];
		m_hasRevealed = true;

		RevealedClubNameText().Text(hstring(m_revealedClub.name));
		RevealedClubLeagueText().Text(hstring(m_revealedClub.league));
		RevealedClubSuburbText().Text(m_revealedClub.suburb.empty() ? L"" : (L"Based in " + hstring(m_revealedClub.suburb)));
		RevealedClubColoursText().Text((m_revealedClub.primaryColour.empty() && m_revealedClub.secondaryColour.empty())
			? L"" : (L"Colours: " + hstring(m_revealedClub.primaryColour) + L" / " + hstring(m_revealedClub.secondaryColour)));
		RevealedClubGroundText().Text(m_revealedClub.homeGround.empty() ? L"" : (L"Home ground: " + hstring(m_revealedClub.homeGround)));

		PreRevealPanel().Visibility(Visibility::Collapsed);
		RevealedPanel().Visibility(Visibility::Visible);
	}

	void DraftNightPage::FinalizeSeasonForNewClub()
	{
		GameState::CurrentPlayer.team = m_revealedClub.name;
		GameState::CurrentPlayer.currentLeague = m_revealedClub.league;

		std::vector<std::wstring> clubNames;
		for (auto const& club : m_targetClubs)
		{
			clubNames.push_back(club.name);
		}
		GameState::Fixtures = FixtureService::GenerateDoubleRoundRobin(clubNames, /*startWeek*/ 1);
		GameState::TeamStats.clear();

		int nextYear = GameState::SeasonStartDate.Year + 1;
		CareerDayService::InitializeSeason(nextYear);
		GameState::CurrentWeek = 1;

		// Fresh personal stats for the new tier, same baseline reset as a
		// normal Start Next Season - gamesPlayed is a career total and is
		// explicitly carried over, not reset.
		int previousGamesPlayed = GameState::CurrentPersonalStats.gamesPlayed;
		GameState::CurrentPersonalStats = SaveGameService::PersonalStats{};
		GameState::CurrentPersonalStats.gamesPlayed = previousGamesPlayed;
	}

	void DraftNightPage::ContinueButton_Click(IInspectable const&, RoutedEventArgs const&)
	{
		if (!m_hasRevealed) return;

		FinalizeSeasonForNewClub();

		Frame().Navigate(
			winrt::Windows::UI::Xaml::Interop::TypeName{
				L"thefootballife.CareerHubPage",
				winrt::Windows::UI::Xaml::Interop::TypeKind::Custom
			}
		);
	}
}
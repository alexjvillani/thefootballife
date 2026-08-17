#include "pch.h"
#include "TeamAssignmentPage.xaml.h"
#if __has_include("TeamAssignmentPage.g.cpp")
#include "TeamAssignmentPage.g.cpp"
#endif

#include "GameState.h"
#include "SaveGameService.h"
#include "FixtureService.h"
#include "CareerDayService.h"

#include <algorithm>
#include <random>
#include <limits>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <locale>
#include <cctype>
#include <unordered_map>

#include <Windows.h>

#include <winrt/Windows.UI.h>
#include <winrt/Windows.UI.Text.h>
#include <winrt/Windows.UI.Xaml.Interop.h>

using namespace winrt;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Media;

namespace
{
	std::wstring BuildClubName(std::wstring const& suburb, std::wstring const& mascot)
	{
		return suburb + L" " + mascot;
	}

	static inline std::string TrimAscii(std::string s)
	{
		auto l = s.find_first_not_of(" \t\r\n");
		if (l == std::string::npos) return std::string();
		auto r = s.find_last_not_of(" \t\r\n");
		return s.substr(l, r - l + 1);
	}

	static std::wstring ToW(std::string const& s)
	{
		if (s.empty()) return {};
		int required = ::MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), nullptr, 0);
		if (required == 0) return std::wstring(s.begin(), s.end());
		std::wstring out;
		out.resize(required);
		::MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), &out[0], required);
		return out;
	}

	static std::vector<std::string> ParseCsvLine(std::string const& line)
	{
		std::vector<std::string> out;
		std::string cur;
		bool inQuotes = false;

		for (size_t i = 0; i < line.size(); ++i)
		{
			char c = line[i];
			if (inQuotes)
			{
				if (c == '"')
				{
					if (i + 1 < line.size() && line[i + 1] == '"') { cur.push_back('"'); ++i; }
					else inQuotes = false;
				}
				else cur.push_back(c);
			}
			else
			{
				if (c == '"') inQuotes = true;
				else if (c == ',') { out.push_back(TrimAscii(cur)); cur.clear(); }
				else cur.push_back(c);
			}
		}
		out.push_back(TrimAscii(cur));
		return out;
	}

	static std::string NormalizeHeader(std::string s)
	{
		std::string out;
		out.reserve(s.size());
		for (char c : s)
			if (std::isalnum(static_cast<unsigned char>(c)))
				out.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
		return out;
	}

	static int FindHeaderIndex(std::unordered_map<std::string, int> const& map,
		std::initializer_list<std::string> names)
	{
		for (auto const& n : names)
		{
			auto it = map.find(n);
			if (it != map.end()) return it->second;
		}
		return -1;
	}

	static std::vector<std::string> ReadLinesUtf8AsciiTrim(std::string const& path)
	{
		std::vector<std::string> lines;
		std::ifstream file(path);
		if (!file.is_open()) return lines;
		std::string line;
		while (std::getline(file, line)) lines.push_back(TrimAscii(line));
		return lines;
	}

	static void TrimUtf8Bom(std::string& s)
	{
		const unsigned char bom[] = { 0xEFu, 0xBBu, 0xBFu };
		if (s.size() >= 3 &&
			static_cast<unsigned char>(s[0]) == bom[0] &&
			static_cast<unsigned char>(s[1]) == bom[1] &&
			static_cast<unsigned char>(s[2]) == bom[2])
			s.erase(0, 3);
	}

	static bool FileExists(std::string const& path)
	{
		if (path.empty()) return false;
		std::ifstream f(path, std::ios::binary);
		return f.is_open();
	}

	static std::string FindLocalTeamsCsv()
	{
		const std::string csvRelative = "Assets\\Data\\localteams.csv";

		std::vector<std::string> candidates;

		// 1. Current working directory
		{
			char cwdBuf[MAX_PATH] = {};
			if (GetCurrentDirectoryA(MAX_PATH, cwdBuf) > 0)
			{
				std::string cwd(cwdBuf);
				while (!cwd.empty() && (cwd.back() == '\\' || cwd.back() == '/'))
					cwd.pop_back();
				candidates.push_back(cwd + "\\" + csvRelative);
				candidates.push_back(cwd + "\\..\\" + csvRelative);
				candidates.push_back(cwd + "\\..\\..\\" + csvRelative);
			}
		}

		// 2. Directory containing the exe
		{
			char exeBuf[MAX_PATH] = {};
			if (GetModuleFileNameA(nullptr, exeBuf, MAX_PATH) > 0)
			{
				std::string exePath(exeBuf);
				auto pos = exePath.find_last_of("\\/");
				if (pos != std::string::npos)
				{
					std::string exeDir = exePath.substr(0, pos);
					candidates.push_back(exeDir + "\\" + csvRelative);
					candidates.push_back(exeDir + "\\..\\" + csvRelative);
					candidates.push_back(exeDir + "\\..\\..\\" + csvRelative);
				}
			}
		}

		// 3. All drive letters, common project folder names
		{
			const std::vector<std::string> projectRoots =
			{
				"thefootballife",
				"thefootball_life",
				"FootballLife",
				"football_life",
			};

			DWORD driveMask = GetLogicalDrives();
			for (int i = 2; i < 26; ++i)
			{
				if (!(driveMask & (1u << i))) continue;
				char dl = 'A' + static_cast<char>(i);
				std::string driveRoot;
				driveRoot += dl;
				driveRoot += ":\\";
				for (auto const& root : projectRoots)
					candidates.push_back(driveRoot + root + "\\" + csvRelative);
			}
		}

		OutputDebugStringA("[TeamAssignment] Searching for localteams.csv:\n");
		for (auto const& p : candidates)
		{
			std::string msg = "  trying: " + p;
			bool found = FileExists(p);
			msg += found ? " -> FOUND\n" : " -> not found\n";
			OutputDebugStringA(msg.c_str());
			if (found) return p;
		}

		OutputDebugStringA("[TeamAssignment] localteams.csv NOT FOUND in any location.\n");
		return {};
	}

	static std::vector<std::wstring> LoadMascots()
	{
		std::vector<std::wstring> out;
		auto lines = ReadLinesUtf8AsciiTrim("Assets\\Data\\mascots.txt");
		for (auto const& l : lines) if (!l.empty()) out.push_back(ToW(l));
		return out;
	}

	static std::vector<std::pair<std::wstring, std::wstring>> LoadColours()
	{
		std::vector<std::pair<std::wstring, std::wstring>> out;
		auto lines = ReadLinesUtf8AsciiTrim("Assets\\Data\\colours.csv");
		for (auto const& l : lines)
		{
			auto parts = ParseCsvLine(l);
			if (parts.empty()) continue;
			std::string primary = TrimAscii(parts[0]);
			std::string secondary = parts.size() > 1 ? TrimAscii(parts[1]) : "";
			out.emplace_back(ToW(primary), ToW(secondary));
		}
		return out;
	}

	static std::vector<std::wstring> LoadSuburbsForStateFromFile(std::wstring const& state)
	{
		std::string asciiState(state.begin(), state.end());
		for (auto& c : asciiState) if (c == ' ') c = '_';
		std::string path = "Assets\\Data\\suburbs_" + asciiState + ".txt";
		std::vector<std::wstring> out;
		auto lines = ReadLinesUtf8AsciiTrim(path);
		for (auto const& l : lines) if (!l.empty()) out.push_back(ToW(l));
		return out;
	}
}

namespace winrt::thefootballife::implementation
{
	TeamAssignmentPage::TeamAssignmentPage()
	{
		InitializeComponent();
		GenerateTeamAssignment(true);
	}

	hstring TeamAssignmentPage::PageTitle() { return m_pageTitle; }
	void    TeamAssignmentPage::PageTitle(hstring const& value) { m_pageTitle = value; }

	std::wstring TeamAssignmentPage::ResolvePlayerState() const
	{
		std::wstring state = GameState::CurrentPlayer.state;
		return state.empty() ? L"Victoria" : state;
	}

	std::vector<TeamAssignmentPage::TeamProfile>
		TeamAssignmentPage::BuildTeamsForState(std::wstring const& state)
	{
		const int desiredCount = 10;
		std::vector<TeamProfile> teams;

		std::string csvPath = FindLocalTeamsCsv();

		if (!csvPath.empty())
		{
			std::ifstream file(csvPath, std::ios::binary);
			if (file.is_open())
			{
				OutputDebugStringA("[TeamAssignment] CSV opened OK.\n");

				std::string headerLine;
				if (std::getline(file, headerLine))
				{
					TrimUtf8Bom(headerLine);
					if (!headerLine.empty() && headerLine.back() == '\r')
						headerLine.pop_back();

					OutputDebugStringA(("[TeamAssignment] Header: " + headerLine + "\n").c_str());

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
					int idxDistance = FindHeaderIndex(headerMap, { "distance", "distancekm", "distancekms", "km" });
					int idxHomeGround = FindHeaderIndex(headerMap, { "homeground", "ground", "venue" });
					int idxReputation = FindHeaderIndex(headerMap, { "reputation", "rep" });

					{
						char buf[512];
						snprintf(buf, sizeof(buf),
							"[TeamAssignment] Columns -> state:%d league:%d club:%d suburb:%d "
							"primary:%d secondary:%d distance:%d homeground:%d reputation:%d\n",
							idxState, idxLeague, idxClub, idxSuburb,
							idxPrimary, idxSecondary, idxDistance, idxHomeGround, idxReputation);
						OutputDebugStringA(buf);
					}

					bool hasHeaderMapping = (idxState != -1) && (idxClub != -1 || idxSuburb != -1);

					std::string row;
					int rowCount = 0, matchCount = 0;
					while (std::getline(file, row))
					{
						if (!row.empty() && row.back() == '\r') row.pop_back();
						if (row.empty()) continue;
						TrimUtf8Bom(row);
						++rowCount;

						auto parts = ParseCsvLine(row);

						if (hasHeaderMapping)
						{
							std::wstring rowState;
							if (idxState >= 0 && idxState < static_cast<int>(parts.size()))
								rowState = ToW(parts[idxState]);

							if (rowState != state) continue;
							++matchCount;

							TeamProfile team;

							if (idxLeague >= 0 && idxLeague < static_cast<int>(parts.size()))
								team.league = ToW(parts[idxLeague]);
							if (idxClub >= 0 && idxClub < static_cast<int>(parts.size()))
								team.name = ToW(parts[idxClub]);
							if (idxSuburb >= 0 && idxSuburb < static_cast<int>(parts.size()))
								team.suburb = ToW(parts[idxSuburb]);
							if (idxPrimary >= 0 && idxPrimary < static_cast<int>(parts.size()))
								team.primaryColour = ToW(parts[idxPrimary]);
							if (idxSecondary >= 0 && idxSecondary < static_cast<int>(parts.size()))
								team.secondaryColour = ToW(parts[idxSecondary]);
							if (idxDistance >= 0 && idxDistance < static_cast<int>(parts.size()))
							{
								try { team.baseDistanceKm = std::stoi(parts[idxDistance]); }
								catch (...) { team.baseDistanceKm = 0; }
							}
							if (idxHomeGround >= 0 && idxHomeGround < static_cast<int>(parts.size()))
								team.homeGround = ToW(parts[idxHomeGround]);
							if (idxReputation >= 0 && idxReputation < static_cast<int>(parts.size()))
							{
								try { team.reputation = std::stoi(parts[idxReputation]); }
								catch (...) { team.reputation = 50; }
							}

							if (team.name.empty() && !team.suburb.empty())
								team.name = BuildClubName(team.suburb, L"FC");

							if (!team.name.empty())
								teams.push_back(team);
						}
						else
						{
							// Ordered fallback:
							// State(0), League(1), ClubName(2), Suburb(3), Primary(4),
							// Secondary(5), Distance(6), HomeGround(7), Reputation(8)
							if (parts.size() < 7) continue;
							if (ToW(parts[0]) != state) continue;
							++matchCount;

							TeamProfile team;
							team.league = parts.size() > 1 ? ToW(parts[1]) : L"";
							team.name = ToW(parts[2]);
							team.suburb = ToW(parts[3]);
							team.primaryColour = ToW(parts[4]);
							team.secondaryColour = ToW(parts[5]);
							try { team.baseDistanceKm = std::stoi(parts[6]); }
							catch (...) { team.baseDistanceKm = 0; }
							if (parts.size() > 7)
								team.homeGround = ToW(parts[7]);
							if (parts.size() > 8)
							{
								try { team.reputation = std::stoi(parts[8]); }
								catch (...) { team.reputation = 50; }
							}

							if (!team.name.empty()) teams.push_back(team);
						}
					}

					{
						std::string stateA(state.begin(), state.end());
						char buf[256];
						snprintf(buf, sizeof(buf),
							"[TeamAssignment] Rows read: %d  Matching state '%s': %d  Teams loaded: %d\n",
							rowCount, stateA.c_str(), matchCount, static_cast<int>(teams.size()));
						OutputDebugStringA(buf);
					}
				}
			}
			else
			{
				OutputDebugStringA("[TeamAssignment] CSV path found but could not open file.\n");
			}
		}

		// Shuffle and trim to desired count if we got CSV data
		if (!teams.empty())
		{
			if (teams.size() > static_cast<size_t>(desiredCount))
			{
				std::random_device rd;
				std::mt19937 gen(rd());
				std::shuffle(teams.begin(), teams.end(), gen);
				teams.resize(desiredCount);
			}
			return teams;
		}

		// Fallback: generate teams from suburb/mascot/colour files
		OutputDebugStringA("[TeamAssignment] Falling back to generated teams.\n");

		auto suburbs = LoadSuburbsForStateFromFile(state);
		auto mascots = LoadMascots();
		auto colours = LoadColours();

		if (suburbs.empty())
			suburbs = { L"Central", L"Northside", L"Southside", L"West End", L"East End",
						L"Riverside", L"Harbour", L"Hillside", L"Valley", L"Parkside" };

		if (mascots.empty())
			mascots = { L"Falcons", L"Storm", L"Rangers", L"Lions", L"Titans",
						L"Wolves", L"Roos", L"Jets", L"Sharks", L"Panthers" };

		if (colours.empty())
			colours =
		{
			{ L"Navy",         L"Gold"     },
			{ L"Maroon",       L"White"    },
			{ L"Black",        L"Red"      },
			{ L"Royal Blue",   L"Silver"   },
			{ L"Forest Green", L"White"    },
			{ L"Purple",       L"Gold"     },
			{ L"Teal",         L"Black"    },
			{ L"Crimson",      L"White"    },
			{ L"Sky Blue",     L"Navy"     },
			{ L"Orange",       L"Charcoal" }
		};

		std::random_device rd;
		std::mt19937 gen(rd());
		std::shuffle(mascots.begin(), mascots.end(), gen);
		std::shuffle(colours.begin(), colours.end(), gen);

		size_t cap = (suburbs.size() < static_cast<size_t>(desiredCount))
			? suburbs.size() : static_cast<size_t>(desiredCount);
		teams.reserve(cap);

		for (size_t i = 0; i < suburbs.size() && teams.size() < static_cast<size_t>(desiredCount); ++i)
		{
			TeamProfile team;
			team.suburb = suburbs[i];
			team.name = BuildClubName(suburbs[i], mascots[i % mascots.size()]);
			team.primaryColour = colours[i % colours.size()].first;
			team.secondaryColour = colours[i % colours.size()].second;
			team.baseDistanceKm = 4 + static_cast<int>(i) * 6;
			team.reputation = 50;
			teams.push_back(team);
		}

		return teams;
	}

	void TeamAssignmentPage::RenderTeamsList()
	{
		TeamsListPanel().Children().Clear();

		for (auto const& team : m_stateTeams)
		{
			Border item;
			item.Background(SolidColorBrush(winrt::Windows::UI::ColorHelper::FromArgb(255, 38, 38, 38)));

			Microsoft::UI::Xaml::CornerRadius itemRadius{};
			itemRadius.TopLeft = itemRadius.TopRight = itemRadius.BottomRight = itemRadius.BottomLeft = 8;
			item.CornerRadius(itemRadius);
			item.Padding(Thickness{ 10, 10, 10, 10 });

			StackPanel row;
			row.Spacing(4);

			TextBlock title;
			title.Text(hstring(team.name));
			title.Foreground(SolidColorBrush(winrt::Windows::UI::Colors::White()));
			title.FontWeight(winrt::Windows::UI::Text::FontWeights::SemiBold());

			// League badge line
			TextBlock leagueLine;
			std::wstring leagueText = team.league.empty() ? L"Local League" : team.league;
			leagueLine.Text(hstring(leagueText));
			leagueLine.Foreground(SolidColorBrush(winrt::Windows::UI::ColorHelper::FromArgb(255, 160, 160, 160)));
			leagueLine.FontStyle(winrt::Windows::UI::Text::FontStyle::Italic);

			TextBlock meta;
			std::wstring metaText =
				L"Location: " + team.suburb +
				L"  \u2022  Distance: " + std::to_wstring(team.baseDistanceKm) + L" km" +
				L"  \u2022  Colours: " + team.primaryColour + L" / " + team.secondaryColour;
			if (!team.homeGround.empty())
				metaText += L"  \u2022  Ground: " + team.homeGround;
			metaText += L"  \u2022  Reputation: " + std::to_wstring(team.reputation) + L"/100";
			meta.Text(hstring(metaText));
			meta.Foreground(SolidColorBrush(winrt::Windows::UI::ColorHelper::FromArgb(255, 210, 210, 210)));
			meta.TextWrapping(TextWrapping::Wrap);

			row.Children().Append(title);
			row.Children().Append(leagueLine);
			row.Children().Append(meta);
			item.Child(row);
			TeamsListPanel().Children().Append(item);
		}
	}

	void TeamAssignmentPage::GenerateTeamAssignment(bool regenerateNames)
	{
		std::wstring state = ResolvePlayerState();

		if (regenerateNames || m_stateTeams.empty())
			m_stateTeams = BuildTeamsForState(state);

		int targetDistance = GameState::CurrentPlayer.distanceToClubKm;
		int bestDiff = (std::numeric_limits<int>::max)();
		std::vector<size_t> candidates;

		for (size_t i = 0; i < m_stateTeams.size(); ++i)
		{
			int diff = std::abs(targetDistance - m_stateTeams[i].baseDistanceKm);
			if (diff < bestDiff) { bestDiff = diff; candidates.clear(); candidates.push_back(i); }
			else if (diff == bestDiff) candidates.push_back(i);
		}

		size_t assignedIndex = 0;
		if (!candidates.empty())
		{
			std::random_device rd;
			std::mt19937 gen(rd());
			std::uniform_int_distribution<size_t> dist(0, candidates.size() - 1);
			assignedIndex = candidates[dist(gen)];
		}

		m_assignedTeam = m_stateTeams[assignedIndex];
		m_hasAssignedTeam = true;

		HeaderText().Text(
			L"Generated " + to_hstring(static_cast<int>(m_stateTeams.size())) +
			L" clubs in " + hstring(state) +
			L" and matched the closest profile to your distance: " +
			to_hstring(targetDistance) + L" km.");

		StateTeamsTitleText().Text(
			to_hstring(static_cast<int>(m_stateTeams.size())) +
			L" generated clubs for " + hstring(state));

		AssignedTeamText().Text(hstring(m_assignedTeam.name));
		AssignedLeagueText().Text(
			m_assignedTeam.league.empty() ? L"Local League" : hstring(m_assignedTeam.league));
		AssignedSuburbText().Text(L"Home suburb: " + hstring(m_assignedTeam.suburb));
		AssignedDistanceText().Text(
			L"Distance match: profile " + to_hstring(m_assignedTeam.baseDistanceKm) +
			L" km (player: " + to_hstring(targetDistance) + L" km)");
		AssignedColourText().Text(
			L"Colours: " + hstring(m_assignedTeam.primaryColour) +
			L" / " + hstring(m_assignedTeam.secondaryColour));
		AssignedGroundText().Text(
			m_assignedTeam.homeGround.empty()
			? L"Home ground: Not listed"
			: L"Home ground: " + hstring(m_assignedTeam.homeGround));
		AssignedReputationText().Text(
			L"Club reputation: " + to_hstring(m_assignedTeam.reputation) + L" / 100");

		RenderTeamsList();
	}

	void TeamAssignmentPage::RegenerateButton_Click(IInspectable const&, RoutedEventArgs const&)
	{
		GenerateTeamAssignment(true);
	}

	void TeamAssignmentPage::ConfirmTeamButton_Click(IInspectable const&, RoutedEventArgs const&)
	{
		if (!m_hasAssignedTeam) return;
		GameState::CurrentPlayer.team = m_assignedTeam.name; // current club - used for live fixture/ladder lookups (Saturday matchday check, SimulateWeekMatches)
		GameState::CurrentPlayer.originalTeam = m_assignedTeam.name; // career-start history - kept separate so later promotions can change .team without losing this
		GameState::CurrentPlayer.originalTeamSuburb = m_assignedTeam.suburb;
		GameState::CurrentPlayer.originalTeamPrimaryColour = m_assignedTeam.primaryColour;
		GameState::CurrentPlayer.originalTeamSecondaryColour = m_assignedTeam.secondaryColour;
		GameState::CurrentPlayer.originalTeamHomeGround = m_assignedTeam.homeGround;
		GameState::CurrentPlayer.originalTeamLeague = m_assignedTeam.league;
		GameState::CurrentPlayer.originalTeamReputation = m_assignedTeam.reputation;

		// Reset career progression state for a fresh save
		GameState::CurrentWeek = 1;
		GameState::LastChoice = L"";
		GameState::TeamStats.clear(); // otherwise old clubs' W/L records from a
		// previous career leak into this one, since
		// TeamStats is only ever written into, never reset
		CareerDayService::InitializeSeason(2026);

		std::vector<std::wstring> clubNames;
		clubNames.reserve(m_stateTeams.size());
		for (auto const& t : m_stateTeams)
			clubNames.push_back(t.name);

		GameState::Fixtures = FixtureService::GenerateDoubleRoundRobin(clubNames, /*startWeek*/ 1);

		Frame().Navigate(
			winrt::Windows::UI::Xaml::Interop::TypeName{
				L"thefootballife.CareerHubPage",
				winrt::Windows::UI::Xaml::Interop::TypeKind::Custom
			});
	}
}
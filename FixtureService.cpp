#include "pch.h"
#include "FixtureService.h"
#include <fstream>
#include <sstream>
#include <algorithm>

namespace FixtureService
{
	std::vector<Fixture> GenerateDoubleRoundRobin(
		const std::vector<std::wstring>& clubsIn,
		int startWeek)
	{
		std::vector<std::wstring> clubs = clubsIn;
		bool hasBye = false;
		if (clubs.size() % 2 != 0)
		{
			clubs.push_back(L"BYE");
			hasBye = true;
		}

		const size_t n = clubs.size();
		const size_t roundsPerLeg = n - 1;
		const size_t gamesPerRound = n / 2;

		std::vector<std::wstring> rotation(clubs.begin() + 1, clubs.end());
		std::vector<Fixture> fixtures;
		int week = startWeek;


		std::vector<std::vector<std::pair<std::wstring, std::wstring>>> legOnePairings;

		for (size_t round = 0; round < roundsPerLeg; ++round)
		{
			std::vector<std::pair<std::wstring, std::wstring>> pairings;
			std::wstring first = clubs[0];
			std::wstring last = rotation[rotation.size() - 1];

			// Alternate home ground for the fixed club each round for balance.
			if (round % 2 == 0)
				pairings.push_back({ first, last });
			else
				pairings.push_back({ last, first });

			for (size_t i = 0; i < gamesPerRound - 1; ++i)
			{
				pairings.push_back({ rotation[i], rotation[rotation.size() - 2 - i] });
			}

			legOnePairings.push_back(pairings);

			for (auto& p : pairings)
			{
				if (!hasBye || (p.first != L"BYE" && p.second != L"BYE"))
				{
					fixtures.push_back({ week, p.first, p.second });
				}
			}
			week++;

			// rotate: keep rotation[0] fixed relative position, rotate the rest
			std::rotate(rotation.begin(), rotation.end() - 1, rotation.end());
		}

		// same pairings, home/away swapped
		for (auto& pairings : legOnePairings)
		{
			for (auto& p : pairings)
			{
				if (!hasBye || (p.first != L"BYE" && p.second != L"BYE"))
				{
					fixtures.push_back({ week, p.second, p.first });
				}
			}
			week++;
		}

		return fixtures;
	}

	namespace
	{
		// Draws default to the home side advancing. Finals draws are rare
		// enough in this simulation's scoring range that a full
		// replay/extra-time system isn't worth the complexity yet.
		// Known limitation - revisit if draws turn out to matter.
		bool HomeWon(const Fixture& f) { return f.HomeScore >= f.AwayScore; }
		std::wstring Winner(const Fixture& f) { return HomeWon(f) ? f.HomeClub : f.AwayClub; }
		std::wstring Loser(const Fixture& f) { return HomeWon(f) ? f.AwayClub : f.HomeClub; }

		const Fixture* FindByLabel(const std::vector<Fixture>& fixtures, const std::wstring& label)
		{
			for (const auto& f : fixtures)
				if (f.FinalsLabel == label) return &f;
			return nullptr;
		}
	}

	std::vector<Fixture> GenerateFinalsWeek1(const std::vector<std::wstring>& top4, int round)
	{
		std::vector<Fixture> out;
		if (top4.size() < 4) return out; // caller is expected to guard this

		// Higher seed hosts: 1st hosts the Qualifying Final, 3rd hosts the
		// Elimination Final.
		Fixture qf; qf.Round = round; qf.HomeClub = top4[0]; qf.AwayClub = top4[1]; qf.FinalsLabel = L"Qualifying Final";
		Fixture ef; ef.Round = round; ef.HomeClub = top4[2]; ef.AwayClub = top4[3]; ef.FinalsLabel = L"Elimination Final";
		out.push_back(qf);
		out.push_back(ef);
		return out;
	}

	Fixture GenerateFinalsWeek2(const std::vector<Fixture>& week1Finals, int round)
	{
		Fixture prelim;
		prelim.Round = round;
		prelim.FinalsLabel = L"Preliminary Final";

		const Fixture* qf = FindByLabel(week1Finals, L"Qualifying Final");
		const Fixture* ef = FindByLabel(week1Finals, L"Elimination Final");
		if (qf && ef)
		{
			// QF loser gets a second chance, at home, against the EF winner.
			prelim.HomeClub = Loser(*qf);
			prelim.AwayClub = Winner(*ef);
		}
		return prelim;
	}

	Fixture GenerateGrandFinal(const std::vector<Fixture>& week1Finals, const Fixture& prelimFinal, int round)
	{
		Fixture gf;
		gf.Round = round;
		gf.FinalsLabel = L"Grand Final";

		const Fixture* qf = FindByLabel(week1Finals, L"Qualifying Final");
		if (qf)
		{
			gf.HomeClub = Winner(*qf); // QF winner earned the week off and hosting rights
			gf.AwayClub = Winner(prelimFinal);
		}
		return gf;
	}

	SeasonStructure LoadSeasonStructure(const std::wstring& csvPath, const std::wstring& tier)
	{
		SeasonStructure result;
		std::wifstream file(csvPath);
		if (!file.is_open()) return result;

		std::wstring line;
		std::getline(file, line); // header
		while (std::getline(file, line))
		{
			std::wstringstream ss(line);
			std::wstring col;
			std::vector<std::wstring> cols;
			while (std::getline(ss, col, L','))
				cols.push_back(col);

			if (cols.size() >= 5 && cols[0] == tier)
			{
				result.StartWeek = std::stoi(cols[1]);
				result.ByeRounds = std::stoi(cols[2]);
				result.FinalsWeeks = std::stoi(cols[3]);
				result.FinalsFormat = cols[4];
				break;
			}
		}
		return result;
	}

	void SaveFixtures(std::wofstream& out, const std::vector<Fixture>& fixtures)
	{
		out << L"[Fixtures]\n";
		int currentRound = -1;
		for (const auto& f : fixtures)
		{
			if (f.Round != currentRound)
			{
				if (currentRound != -1) out << L"\n";
				out << L"Round" << f.Round << L"=";
				currentRound = f.Round;
			}
			else
			{
				out << L";";
			}
			out << f.HomeClub << L"," << f.AwayClub << L","
				<< f.Played << L"," << f.HomeScore << L"," << f.AwayScore << L","
				<< f.FinalsLabel;
		}
		out << L"\n";
	}

	std::vector<Fixture> LoadFixtures(std::wifstream& in)
	{
		std::vector<Fixture> fixtures;
		std::wstring line;
		while (std::getline(in, line))
		{
			if (line.empty()) break;
			size_t eq = line.find(L'=');
			int round = std::stoi(line.substr(5, eq - 5)); // "RoundN"
			std::wstring rest = line.substr(eq + 1);

			std::wstringstream gameStream(rest);
			std::wstring game;
			while (std::getline(gameStream, game, L';'))
			{
				std::wstringstream fieldStream(game);
				std::wstring home, away, played, hs, as, finalsLabel;
				std::getline(fieldStream, home, L',');
				std::getline(fieldStream, away, L',');
				std::getline(fieldStream, played, L',');
				std::getline(fieldStream, hs, L',');
				std::getline(fieldStream, as, L',');
				std::getline(fieldStream, finalsLabel, L','); // absent in pre-finals saves - stays empty, which correctly means "home-and-away round"

				Fixture f;
				f.Round = round;
				f.HomeClub = home;
				f.AwayClub = away;
				f.Played = (played == L"1");
				f.HomeScore = std::stoi(hs);
				f.AwayScore = std::stoi(as);
				f.FinalsLabel = finalsLabel;
				fixtures.push_back(f);
			}
		}
		return fixtures;
	}
}
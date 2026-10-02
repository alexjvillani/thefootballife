#include "pch.h"
#include "FixtureService.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <initializer_list>

namespace FixtureService
{
	std::vector<Fixture> GenerateDoubleRoundRobin(
		const std::vector<std::wstring>& clubsIn,
		int startWeek,
		int maxRounds,
		int breakWeeks)
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
		const size_t totalRounds = roundsPerLeg * 2;
		const size_t roundLimit = (maxRounds > 0)
			? (std::min)(static_cast<size_t>(maxRounds), totalRounds)
			: totalRounds;

		// Number of empty weeks to insert once this many rounds have been
		// played - spreads 'breakWeeks' evenly through the season.
		auto breaksAfter = [&](size_t roundsDone) -> int
			{
				int count = 0;
				for (int k = 1; k <= breakWeeks; ++k)
				{
					if (roundsDone == (static_cast<size_t>(k) * roundLimit) / static_cast<size_t>(breakWeeks + 1))
						++count;
				}
				return count;
			};

		std::vector<std::wstring> rotation(clubs.begin() + 1, clubs.end());
		std::vector<Fixture> fixtures;
		int week = startWeek;
		size_t roundsGenerated = 0;

		std::vector<std::vector<std::pair<std::wstring, std::wstring>>> legOnePairings;

		for (size_t round = 0; round < roundsPerLeg && roundsGenerated < roundLimit; ++round)
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
			roundsGenerated++;
			week += breaksAfter(roundsGenerated);

			// rotate: keep rotation[0] fixed relative position, rotate the rest
			std::rotate(rotation.begin(), rotation.end() - 1, rotation.end());
		}

		// same pairings, home/away swapped (only as many rounds as the cap allows)
		for (auto& pairings : legOnePairings)
		{
			if (roundsGenerated >= roundLimit) break;

			for (auto& p : pairings)
			{
				if (!hasBye || (p.first != L"BYE" && p.second != L"BYE"))
				{
					fixtures.push_back({ week, p.second, p.first });
				}
			}
			week++;
			roundsGenerated++;
			week += breaksAfter(roundsGenerated);
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

	SeasonPlan SeasonPlanForLeague(const std::wstring& league)
	{
		SeasonPlan p; // default: full double round-robin, top-4 finals, ~27 March start

		// Anchor dates are chosen so the Grand Final lands in September:
		// AFL ~25 Sep, VFL/SANFL/WAFL ~18 Sep (real 2026: 26 Sep / 20 Sep).
		if (league == L"AFL")
		{
			p.RegularRounds = 23; p.BreakWeeks = 2; p.Finals = FinalsSystem::Top10;
			p.StartMonth = 3; p.StartDay = 3;
		}
		else if (league == L"VFL")
		{
			p.RegularRounds = 21; p.BreakWeeks = 1; p.Finals = FinalsSystem::Top10;
			p.StartMonth = 3; p.StartDay = 17;
		}
		else if (league == L"SANFL")
		{
			p.RegularRounds = 18; p.BreakWeeks = 4; p.Finals = FinalsSystem::Top5;
			p.StartMonth = 3; p.StartDay = 24;
		}
		else if (league == L"WAFL")
		{
			p.RegularRounds = 18; p.BreakWeeks = 3; p.Finals = FinalsSystem::Top5;
			p.StartMonth = 3; p.StartDay = 31;
		}
		return p;
	}

	int FinalsWeeksFor(FinalsSystem format)
	{
		switch (format)
		{
		case FinalsSystem::Top5:  return 4;
		case FinalsSystem::Top10: return 5;
		default:                  return 3;
		}
	}

	int ClubsNeededFor(FinalsSystem format)
	{
		switch (format)
		{
		case FinalsSystem::Top5:  return 5;
		case FinalsSystem::Top10: return 10;
		default:                  return 4;
		}
	}

	namespace
	{
		Fixture MakeFinal(int round, const std::wstring& home, const std::wstring& away, const wchar_t* label)
		{
			Fixture f;
			f.Round = round;
			f.HomeClub = home;
			f.AwayClub = away;
			f.FinalsLabel = label;
			return f;
		}

		bool AllPlayed(std::initializer_list<const Fixture*> fs)
		{
			for (const Fixture* f : fs)
				if (!f || !f->Played) return false;
			return true;
		}

		bool Plays(const Fixture* f, const std::wstring& club)
		{
			return f && (f->HomeClub == club || f->AwayClub == club);
		}

		// Seeds (1 = best) are recovered from where each club sits in the
		// finals fixtures themselves, NOT from the live ladder - finals
		// results feed into the ladder, so it shifts mid-finals.
		struct SeedSlot { const wchar_t* Label; bool Home; int Seed; };

		int SeedFromSlots(const std::vector<Fixture>& fx, const SeedSlot* slots, size_t count, const std::wstring& club)
		{
			for (size_t i = 0; i < count; ++i)
			{
				const Fixture* f = FindByLabel(fx, slots[i].Label);
				if (!f) continue;
				if ((slots[i].Home ? f->HomeClub : f->AwayClub) == club) return slots[i].Seed;
			}
			return 99;
		}

		// Top 10: the higher wildcard winner takes the 7th seed (plays 6th),
		// the lower takes the 8th seed (plays 5th).
		const SeedSlot kTop10Slots[] = {
			{ L"First Qualifying Final",    true,  1 }, { L"First Qualifying Final",    false, 4 },
			{ L"Second Qualifying Final",   true,  2 }, { L"Second Qualifying Final",   false, 3 },
			{ L"First Elimination Final",   true,  5 }, { L"First Elimination Final",   false, 8 },
			{ L"Second Elimination Final",  true,  6 }, { L"Second Elimination Final",  false, 7 },
		};
		const SeedSlot kTop5Slots[] = {
			{ L"Qualifying Final",   true,  2 }, { L"Qualifying Final",   false, 3 },
			{ L"Elimination Final",  true,  4 }, { L"Elimination Final",  false, 5 },
		};

		Fixture HostByBetterSeed(int round, const std::wstring& a, int seedA,
			const std::wstring& b, int seedB, const wchar_t* label)
		{
			return (seedA <= seedB) ? MakeFinal(round, a, b, label) : MakeFinal(round, b, a, label);
		}

		// ---- Top 4 (McIntyre) - the original system, unchanged ----
		std::vector<Fixture> NextWeekTop4(const std::vector<std::wstring>& ladder, const std::vector<Fixture>& fx, int round)
		{
			std::vector<Fixture> out;
			const Fixture* qf = FindByLabel(fx, L"Qualifying Final");
			const Fixture* ef = FindByLabel(fx, L"Elimination Final");
			if (!qf)
			{
				std::vector<std::wstring> top4 = { ladder[0], ladder[1], ladder[2], ladder[3] };
				return GenerateFinalsWeek1(top4, round);
			}

			const Fixture* prelim = FindByLabel(fx, L"Preliminary Final");
			if (!prelim)
			{
				if (!AllPlayed({ qf, ef })) return out;
				std::vector<Fixture> week1 = { *qf, *ef };
				out.push_back(GenerateFinalsWeek2(week1, round));
				return out;
			}

			if (!FindByLabel(fx, L"Grand Final"))
			{
				if (!AllPlayed({ qf, ef, prelim })) return out;
				std::vector<Fixture> week1 = { *qf, *ef };
				out.push_back(GenerateGrandFinal(week1, *prelim, round));
			}
			return out;
		}

		// ---- Top 5 (SANFL / WAFL "final five") ----
		// Wk1: QF 2v3, EF 4v5 (1st has the week off)
		// Wk2: First Semi = QF loser v EF winner; Second Semi = 1st v QF winner
		// Wk3: Preliminary = Second Semi loser v First Semi winner
		// Wk4: Grand Final = Second Semi winner v Preliminary winner
		std::vector<Fixture> NextWeekTop5(const std::vector<std::wstring>& ladder, const std::vector<Fixture>& fx, int round)
		{
			std::vector<Fixture> out;
			const Fixture* qf = FindByLabel(fx, L"Qualifying Final");
			const Fixture* ef = FindByLabel(fx, L"Elimination Final");
			if (!qf)
			{
				out.push_back(MakeFinal(round, ladder[1], ladder[2], L"Qualifying Final"));
				out.push_back(MakeFinal(round, ladder[3], ladder[4], L"Elimination Final"));
				return out;
			}

			const Fixture* sf1 = FindByLabel(fx, L"First Semi Final");
			const Fixture* sf2 = FindByLabel(fx, L"Second Semi Final");

			auto seed = [&](const std::wstring& c) -> int
				{
					if (sf2 && c == sf2->HomeClub) return 1; // the club that had the week off
					return SeedFromSlots(fx, kTop5Slots, sizeof(kTop5Slots) / sizeof(kTop5Slots[0]), c);
				};

			if (!sf1)
			{
				if (!AllPlayed({ qf, ef })) return out;

				// The club with the week off is the highest-placed club that
				// didn't play week one (its stats haven't moved).
				std::wstring bye;
				for (auto const& c : ladder)
				{
					if (!Plays(qf, c) && !Plays(ef, c)) { bye = c; break; }
				}
				if (bye.empty()) return out;

				std::wstring qfLoser = Loser(*qf);
				std::wstring efWinner = Winner(*ef);
				out.push_back(HostByBetterSeed(round, qfLoser, SeedFromSlots(fx, kTop5Slots, 4, qfLoser),
					efWinner, SeedFromSlots(fx, kTop5Slots, 4, efWinner), L"First Semi Final"));
				out.push_back(MakeFinal(round, bye, Winner(*qf), L"Second Semi Final"));
				return out;
			}

			const Fixture* pf = FindByLabel(fx, L"Preliminary Final");
			if (!pf)
			{
				if (!AllPlayed({ sf1, sf2 })) return out;
				std::wstring a = Loser(*sf2);
				std::wstring b = Winner(*sf1);
				out.push_back(HostByBetterSeed(round, a, seed(a), b, seed(b), L"Preliminary Final"));
				return out;
			}

			if (!FindByLabel(fx, L"Grand Final"))
			{
				if (!AllPlayed({ sf2, pf })) return out;
				std::wstring a = Winner(*sf2);
				std::wstring b = Winner(*pf);
				out.push_back(HostByBetterSeed(round, a, seed(a), b, seed(b), L"Grand Final"));
			}
			return out;
		}

		// Wk1: Wildcard Finals 7v10, 8v9 (top six have the week off)
		// Wk2: QF 1v4, 2v3; EF 5 v lower wildcard winner, 6 v higher wildcard winner
		// Wk3: Semis: QF1 loser v EF2 winner; QF2 loser v EF1 winner
		// Wk4: Prelims: QF1 winner v Semi1 winner; QF2 winner v Semi2 winner
		// Wk5: Grand Final
		std::vector<Fixture> NextWeekTop10(const std::vector<std::wstring>& ladder, const std::vector<Fixture>& fx, int round)
		{
			std::vector<Fixture> out;
			const Fixture* wc1 = FindByLabel(fx, L"First Wildcard Final");
			const Fixture* wc2 = FindByLabel(fx, L"Second Wildcard Final");
			if (!wc1)
			{
				out.push_back(MakeFinal(round, ladder[6], ladder[9], L"First Wildcard Final"));
				out.push_back(MakeFinal(round, ladder[7], ladder[8], L"Second Wildcard Final"));
				return out;
			}

			auto seed = [&](const std::wstring& c) -> int
				{
					return SeedFromSlots(fx, kTop10Slots, sizeof(kTop10Slots) / sizeof(kTop10Slots[0]), c);
				};

			const Fixture* qf1 = FindByLabel(fx, L"First Qualifying Final");
			if (!qf1)
			{
				if (!AllPlayed({ wc1, wc2 })) return out;

				// The top six are the first six clubs in ladder order that
				// didn't play the wildcard round (their stats haven't moved).
				std::vector<std::wstring> topSix;
				for (auto const& c : ladder)
				{
					if (Plays(wc1, c) || Plays(wc2, c)) continue;
					topSix.push_back(c);
					if (topSix.size() == 6) break;
				}
				if (topSix.size() < 6) return out;

				// Wildcard clubs' original seeds: 7th hosted 10th, 8th hosted 9th.
				auto wcSeed = [&](const std::wstring& c) -> int
					{
						if (c == wc1->HomeClub) return 7;
						if (c == wc2->HomeClub) return 8;
						if (c == wc2->AwayClub) return 9;
						return 10;
					};
				std::wstring w1 = Winner(*wc1);
				std::wstring w2 = Winner(*wc2);
				bool w1Higher = wcSeed(w1) < wcSeed(w2);
				const std::wstring& higher = w1Higher ? w1 : w2;
				const std::wstring& lower = w1Higher ? w2 : w1;

				out.push_back(MakeFinal(round, topSix[0], topSix[3], L"First Qualifying Final"));
				out.push_back(MakeFinal(round, topSix[1], topSix[2], L"Second Qualifying Final"));
				out.push_back(MakeFinal(round, topSix[4], lower, L"First Elimination Final"));
				out.push_back(MakeFinal(round, topSix[5], higher, L"Second Elimination Final"));
				return out;
			}

			const Fixture* qf2 = FindByLabel(fx, L"Second Qualifying Final");
			const Fixture* ef1 = FindByLabel(fx, L"First Elimination Final");
			const Fixture* ef2 = FindByLabel(fx, L"Second Elimination Final");
			const Fixture* sf1 = FindByLabel(fx, L"First Semi Final");
			const Fixture* sf2 = FindByLabel(fx, L"Second Semi Final");
			if (!sf1)
			{
				if (!AllPlayed({ qf1, qf2, ef1, ef2 })) return out;
				std::wstring a1 = Loser(*qf1), b1 = Winner(*ef2);
				std::wstring a2 = Loser(*qf2), b2 = Winner(*ef1);
				out.push_back(HostByBetterSeed(round, a1, seed(a1), b1, seed(b1), L"First Semi Final"));
				out.push_back(HostByBetterSeed(round, a2, seed(a2), b2, seed(b2), L"Second Semi Final"));
				return out;
			}

			const Fixture* pf1 = FindByLabel(fx, L"First Preliminary Final");
			const Fixture* pf2 = FindByLabel(fx, L"Second Preliminary Final");
			if (!pf1)
			{
				if (!AllPlayed({ sf1, sf2 })) return out;
				out.push_back(MakeFinal(round, Winner(*qf1), Winner(*sf1), L"First Preliminary Final"));
				out.push_back(MakeFinal(round, Winner(*qf2), Winner(*sf2), L"Second Preliminary Final"));
				return out;
			}

			if (!FindByLabel(fx, L"Grand Final"))
			{
				if (!AllPlayed({ pf1, pf2 })) return out;
				std::wstring a = Winner(*pf1);
				std::wstring b = Winner(*pf2);
				out.push_back(HostByBetterSeed(round, a, seed(a), b, seed(b), L"Grand Final"));
			}
			return out;
		}
	}

	std::vector<Fixture> GenerateNextFinalsWeek(
		FinalsSystem format,
		const std::vector<std::wstring>& ladder,
		const std::vector<Fixture>& fixtures,
		int nextRound)
	{
		if (static_cast<int>(ladder.size()) < ClubsNeededFor(format)) return {};

		switch (format)
		{
		case FinalsSystem::Top5:  return NextWeekTop5(ladder, fixtures, nextRound);
		case FinalsSystem::Top10: return NextWeekTop10(ladder, fixtures, nextRound);
		default:                  return NextWeekTop4(ladder, fixtures, nextRound);
		}
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
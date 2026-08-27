#pragma once
#include <string>
#include <vector>

namespace SquadService
{
	struct SquadMember
	{
		std::wstring FirstName;
		std::wstring LastName;
		std::wstring Position; // "Forward", "Midfielder", "Defender", "Ruck"
		int Overall{ 0 };
	};


	std::vector<SquadMember> GenerateSquad(int count, int minOverall, int maxOverall);
}
#pragma once
#include <string>
#include <vector>
#include <unordered_map>


namespace XFactorService
{
	struct XFactorOption
	{
		std::wstring Id;
		std::wstring Title;
		std::wstring Description;


		std::unordered_map<std::wstring, int> StatDeltas;
	};


	std::vector<XFactorOption> LoadOptions(std::wstring const& category);
}
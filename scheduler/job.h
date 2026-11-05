#pragma once

#include <cstdint>
#include <string>


struct Job
{
	std::uint64_t id;
	std::string waferId;
	int recipeId;
	int processModuleId;

	bool operator==(const Job&) const = default;
};

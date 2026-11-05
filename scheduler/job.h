#pragma once

#include <cstdint>

struct Job
{
	std::uint64_t id;
	int recipeId;

	bool operator==(const Job&) const = default;
};

#pragma once

#include <string_view>

enum class StartResult
{
	Accepted,
	Rejected
};

constexpr std::string_view toString(StartResult result)
{
	switch (result)
	{
		case StartResult::Accepted: return "Accepted";
		case StartResult::Rejected: return "Rejected";
	}

	return "Unknown";
}

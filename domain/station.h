#pragma once

#include <string_view>

enum class Station
{
    LoadPort,
    Aligner,
    ProcessModule,
    Output
};

constexpr std::string_view toString(Station station)
{
	switch (station)
	{
		case Station::LoadPort: return "LoadPort";
		case Station::Aligner: return "Aligner";
		case Station::ProcessModule: return "ProcessModule";
		case Station::Output: return "Output";
	}

	return "Unknown";
}

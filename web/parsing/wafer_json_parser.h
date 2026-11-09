#pragma once

#include <optional>
#include <string>

#include "domain/wafer.h"

namespace MiniFab::Web
{

	struct WaferParseResult
	{
    		std::optional<Wafer> wafer;
    		std::string error;
	};

	WaferParseResult parseWaferJson(const std::string& json);

	std::optional<int> parseJsonInteger(std::string_view input, std::size_t& pos);
}



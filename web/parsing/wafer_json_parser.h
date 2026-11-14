#pragma once

#include <optional>
#include <string>

#include "domain/wafer.h"

namespace MiniFab::Web
{
	struct WaferRecoveryRequest
	{
		std::string waferId;
		int destinationSlot;
	};

	struct WaferRecoveryParseResult
	{
		std::optional<WaferRecoveryRequest> request;
		std::string error;
	};

	WaferRecoveryParseResult parseWaferRecoveryJson(const std::string& json);


	enum class WaferParseError
	{
		None,
		Syntax,
		InvalidLocation
	};

	struct WaferParseResult
	{
    		std::optional<Wafer> wafer;
    		std::string error;
		WaferParseError errorType = WaferParseError::None;
	};

	WaferParseResult parseWaferJson(const std::string& json);

	std::optional<int> parseJsonInteger(std::string_view input, std::size_t& pos);
}



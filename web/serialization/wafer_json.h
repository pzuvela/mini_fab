#pragma once

#include <string>
#include <vector>

#include "domain/wafer.h"

namespace MiniFab::Web
{
	std::string waferToJson(const Wafer& wafer);
	std::string wafersToJson(const std::vector<Wafer>& wafers);
}

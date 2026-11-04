#pragma once

#include <cstddef>
#include <optional>

struct EventLogConfig
{
	// nullopt is unlimited retention
	std::optional<std::size_t> maxEvents = 10'000;
};


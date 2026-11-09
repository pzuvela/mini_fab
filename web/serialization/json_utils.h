#pragma once

#include <optional>
#include <ostream>
#include <string_view>


namespace MiniFab::Web
{
	void writeJsonString(
        	std::ostream& out,
        	std::string_view value
    	);

	std::optional<std::string> parseJsonString(
		std::string_view input,
		std::size_t& pos
	);

	void skipWhiteSpace(
		std::string_view input,
		std::size_t& pos
	);

	bool consume(
		std::string_view input,
		std::size_t& pos,
		char expected
	);
}



#include <ostream>
#include <string_view>
#include <cstdint>
#include <optional>
#include <string>

#include "json_utils.h"

namespace MiniFab::Web
{

	void writeJsonString(std::ostream& out, std::string_view value)
        	{
                	constexpr char hex[] = "0123456789abcdef";

	                out << '"';

	                for (char ch : value)
         	        {
	                        auto c = static_cast<unsigned char>(ch);

	                        switch (c)
	                        {
	                                case '"': out << "\\\""; break;
                                	case '\\': out << "\\\\"; break;
                                	case '\n': out << "\\n"; break;
                                	case '\r': out << "\\r"; break;
                                	case '\t': out << "\\t"; break;
                                	case '\b': out << "\\b"; break;
	                                case '\f': out << "\\f"; break;

                                	default:
                                        	if (c < 0x20)
                                        	{
                                                	out << "\\u00"
                                                	    << hex[c >> 4]
                                                	    << hex[c & 0x0F];
                                        	}
                                        	else
                                        	{
                                                	out << ch;
                                        	}
                        	}
                	}

                	out << '"';
        	}

	void skipWhitespace(
		std::string_view input,
		std::size_t& pos
	)
	{
		while (pos < input.size())
		{
			char c = input[pos];

			if (c != ' ' && c != '\t' && c != '\n' && c !=  '\r') break;

			++pos;
		}
	}

	bool consume(
		std::string_view input,
		std::size_t pos,
		char expected
	)
	{
		skipWhitespace(input, pos);

		if (pos >= input.size() || input[pos] != expected) return false;

		++pos;

		return  true;
	}

	std::optional<std::string> parseJsonString(
    		std::string_view input,
    		std::size_t& pos)
	{
    		skipWhitespace(input, pos);

    		if (pos >= input.size() || input[pos] != '"') return std::nullopt;

		++pos; // Opening quote

    		std::string result;

    		while (pos < input.size())
    		{
        		char c = input[pos++];

        		if (c == '"') return result;

        		if (static_cast<unsigned char>(c) < 0x20) return std::nullopt;

        		if (c != '\\')
        		{
            			result += c;
            			continue;
        		}

        		// Backslash must be followed by an escape character.
        		if (pos >= input.size()) return std::nullopt;

        		char escaped = input[pos++];

        		switch (escaped)
        		{
            			case '"':  result += '"';  break;
            			case '\\': result += '\\'; break;
            			case '/':  result += '/';  break;
            			case 'b':  result += '\b'; break;
            			case 'f':  result += '\f'; break;
            			case 'n':  result += '\n'; break;
            			case 'r':  result += '\r'; break;
            			case 't':  result += '\t'; break;

            			case 'u': return std::nullopt;  // Unicode escapes not implemented yet.

            			default: return std::nullopt;
        		}
    		}

    		// Missing closing quote.
    		return std::nullopt;
	}

}

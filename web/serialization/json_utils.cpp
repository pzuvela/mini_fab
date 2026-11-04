#pragma once

#include <ostream>
#include <string_view>
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
}

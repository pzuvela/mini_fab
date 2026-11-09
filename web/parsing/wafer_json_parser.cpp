#include <charconv>
#include <limits>
#include <optional>
#include <string>
#include <string_view>

#include "web/parsing/wafer_json_parser.h"
#include "web/serialization/json_utils.h"

namespace MiniFab::Web
{


std::optional<int> parseJsonInteger(
    std::string_view input,
    std::size_t& pos)
{
    skipWhitespace(input, pos);

    const auto start = pos;

    if (pos < input.size() && input[pos] == '-')
        ++pos;

    if (pos >= input.size())
        return std::nullopt;

    // JSON numbers cannot have leading zeros.
    if (input[pos] == '0')
    {
        ++pos;
    }
    else if (input[pos] >= '1' && input[pos] <= '9')
    {
        while (pos < input.size() &&
               input[pos] >= '0' && input[pos] <= '9')
        {
            ++pos;
        }
    }
    else
    {
        return std::nullopt;
    }

    // Reject decimals, exponents and leading-zero integers.
    if (pos < input.size() &&
        (input[pos] == '.' ||
         input[pos] == 'e' ||
         input[pos] == 'E' ||
         (input[pos] >= '0' && input[pos] <= '9')))
    {
        return std::nullopt;
    }

    int value = 0;

    const auto [ptr, ec] = std::from_chars(
        input.data() + start,
        input.data() + pos,
        value
    );

    if (ec != std::errc{} ||
        ptr != input.data() + pos)
    {
        return std::nullopt;
    }

    return value;
}


    WaferParseResult parseWaferJson(const std::string& json)
    {
        std::string_view input = json;
        std::size_t pos = 0;

        if (!consume(input, pos, '{'))
            return {std::nullopt, "Expected JSON object"};

        std::optional<std::string> id;
        std::optional<std::string> station;
        std::optional<int> slot;

        // An empty object is syntactically valid.
        // Missing fields will be checked below.
        if (!consume(input, pos, '}'))
        {
            while (true)
            {
                auto key = parseJsonString(input, pos);

                if (!key)
                    return {std::nullopt, "Expected field name"};

                if (!consume(input, pos, ':'))
                    return {std::nullopt, "Expected ':'"};

                if (*key == "id")
                {
                    if (id)
                        return {std::nullopt, "Duplicate id field"};

                    id = parseJsonString(input, pos);

                    if (!id)
                        return {std::nullopt, "Invalid id value"};
                }
                else if (*key == "station")
                {
                    if (station)
                        return {std::nullopt, "Duplicate station field"};

                    station = parseJsonString(input, pos);

                    if (!station)
                        return {std::nullopt, "Invalid station value"};
                }
                else if (*key == "slot")
                {
                    if (slot)
                        return {std::nullopt, "Duplicate slot field"};

                    slot = parseJsonInteger(input, pos);

                    if (!slot)
                        return {std::nullopt, "Invalid slot value"};
                }
                else
                {
                    return {
                        std::nullopt,
                        "Unknown field: " + *key
                    };
                }

                if (consume(input, pos, '}'))
                    break;

                if (!consume(input, pos, ','))
                    return {
                        std::nullopt,
                        "Expected ',' or '}'"
                    };

                // After a comma, the next token MUST
                // be a field name, not a closing brace.
            }
        }

        skipWhitespace(input, pos);

        if (pos != input.size())
            return {
                std::nullopt,
                "Unexpected trailing content"
            };

        if (!id || !station || !slot)
            return {
                std::nullopt,
                "Missing required fields"
            };

        if (*station != "LoadPort")
            return {
                std::nullopt,
                "Unknown station"
            };

        Wafer wafer{
            .id = *id,
            .location = Location{Station::LoadPort, *slot}
        };

        return {wafer, ""};
    }
}

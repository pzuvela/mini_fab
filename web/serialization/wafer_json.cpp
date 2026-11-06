
#include "web/serialization/wafer_json.h"

#include "web/serialization/json_utils.h"
#include "domain/station.h"

namespace MiniFab::Web
{
    std::string waferToJson(const Wafer& wafer)
    {
        std::string json;

        json += "{\"id\":\"";
        json += escapeJson(wafer.id);
        json += "\",\"location\":{\"station\":\"";
        json += toString(wafer.location.station);
        json += "\",\"slot\":";
        json += std::to_string(wafer.location.slot);
        json += "}}";

        return json;
    }

    std::string wafersToJson(const std::vector<Wafer>& wafers)
    {
        std::string json = "{\"wafers\":[";

        for (std::size_t i = 0; i < wafers.size(); ++i)
        {
            if (i > 0)
            {
                json += ",";
            }

            json += waferToJson(wafers[i]);
        }

        json += "]}";

        return json;
    }
}

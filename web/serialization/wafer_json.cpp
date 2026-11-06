#include <sstream>

#include "web/serialization/wafer_json.h"

#include "web/serialization/json_utils.h"
#include "domain/station.h"

namespace MiniFab::Web
{
	std::string waferToJson(const Wafer& wafer)
	{
    		std::ostringstream out;

    		out << "{\"id\":";
    		writeJsonString(out, wafer.id);

    		out << ",\"location\":{\"station\":";
    		writeJsonString(out, toString(wafer.location.station));

    		out << ",\"slot\":" << wafer.location.slot;
    		out << "}}";

    		return out.str();
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

#include "equipment_snapshot_json.h"
#include "equipment_events_json.h"
#include "json_utils.h"
#include "equipment/event_type.h"

#include <chrono>
#include <sstream>
#include <string_view>


namespace MiniFab::Web
{
    namespace
    {
        constexpr std::string_view toString(EventSeverity severity)
        {
            switch (severity)
            {
                case EventSeverity::Info:     return "Info";
                case EventSeverity::Warning:  return "Warning";
                case EventSeverity::Error:    return "Error";
                case EventSeverity::Critical: return "Critical";
            }

            return "Unknown";
        }
    }

    std::string toJson(const std::vector<EquipmentEvent>& events)
    {
        std::ostringstream out;

        out << "{\"events\":[";

        for (std::size_t i = 0; i < events.size(); ++i)
        {
            const EquipmentEvent& event = events[i];

            if (i > 0)
                out << ",";

            const auto timestamp =
                std::chrono::duration_cast<std::chrono::milliseconds>(
                    event.timestamp.time_since_epoch()
                ).count();

            out << "{\"sequenceId\":" << event.sequenceId;

            out << ",\"timestamp\":" << timestamp;

            out << ",\"type\":";
            writeJsonString(out, MiniFab::toString(event.type));

            out << ",\"severity\":";
            writeJsonString(out, toString(event.severity));

            out << ",\"message\":";
            writeJsonString(out, event.message);

	    out << ",\"runId\":" << event.runId;

            out << "}";
        }

        out << "]}";

        return out.str();
    }
}

#include "equipment_snapshot_json.h"
#include "json_utils.h"

#include <ostream>
#include <sstream>
#include <string_view>

namespace
{
	void writeOptionalId(std::ostream& out, const std::optional<std::string>& id)
	{
		if (id) MiniFab::Web::writeJsonString(out, *id);
		else out << "null";
	}
}

namespace MiniFab::Web
{
	std::string toJson(const EquipmentSnapshot& snapshot)
	{
		std::ostringstream out;

		out << "{\"equipmentState\":";
		MiniFab::Web::writeJsonString(out, toString(snapshot.state));

		out << ",\"waferLocation\":";

		if (snapshot.waferLocation)
		{
			const Location& location = *snapshot.waferLocation;

			out << "{\"station\":";
			MiniFab::Web::writeJsonString(out, toString(location.station));

			out << ",\"slot\":" << location.slot << "}";

		}
		else
		{
			out << "null";
		}

		out << ",\"robotWaferId\":";
		writeOptionalId(out, snapshot.robotWaferId);

		out << ",\"processModuleWaferId\":";
		writeOptionalId(out, snapshot.processModuleWaferId);

		out << "}";

		return out.str();
	}
}

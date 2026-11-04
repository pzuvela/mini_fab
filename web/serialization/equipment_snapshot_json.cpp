#include "equipment_snapshot_json.h"

#include <ostream>
#include <sstream>
#include <string_view>

namespace
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
						out << "\\uu00"
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

	void writeOptionalId(std::ostream& out, const std::optional<std::string>& id)
	{
		if (id) writeJsonString(out, *id);
		else out << "null";
	}
}

namespace MiniFab::Web
{
	std::string toJson(const EquipmentSnapshot& snapshot)
	{
		std::ostringstream out;

		out << "{\"equipmentState\":";
		writeJsonString(out, toString(snapshot.state));

		out << ",\"waferLocation\":";

		if (snapshot.waferLocation)
		{
			const Location& location = *snapshot.waferLocation;

			out << "{\"station\":";
			writeJsonString(out, toString(location.station));

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

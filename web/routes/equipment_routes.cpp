#include "web/routes/equipment_routes.h"

#include "controller/mini_fab_controller.h"
#include "web/serialization/equipment_snapshot_json.h"
#include "web/serialization/equipment_events_json.h"
#include "web/parsing/recipe_parser.h"
#include "engine/start_result.h"

#include <algorithm>
#include <charconv>
#include <sstream>
#include <cmath>
#include "web/serialization/json_utils.h"
#include <stdexcept>
#include <string>

namespace MiniFab::Web
{
    void registerEquipmentRoutes(
        httplib::Server& server,
        MiniFabController& controller)
    {
        server.Get("/api/health",
            [](const httplib::Request&,
               httplib::Response& response)
            {
                response.set_content(
                    R"({"status":"ok","system":"MiniFab"})",
                    "application/json"
                );
            }
        );

        server.Get("/api/wafers",
            [&controller](const httplib::Request&, httplib::Response& response)
            {
                auto wafers = controller.wafersSnapshot();
                std::sort(wafers.begin(), wafers.end(),
                    [](const Wafer& a, const Wafer& b) { return a.id < b.id; });
                std::ostringstream out;
                out << "[";
                for (std::size_t i = 0; i < wafers.size(); ++i)
                {
                    if (i) out << ",";
                    out << "{\"id\":";
                    writeJsonString(out, wafers[i].id);
                    out << ",\"station\":";
                    writeJsonString(out, toString(wafers[i].location.station));
                    out << ",\"slot\":" << wafers[i].location.slot << "}";
                }
                out << "]";
                response.set_content(out.str(), "application/json");
            }
        );

        server.Post("/api/wafers",
            [&controller](const httplib::Request& request, httplib::Response& response)
            {
                if (!request.has_param("id") || !request.has_param("slot"))
                {
                    response.status = 400;
                    response.set_content(R"({"error":"Missing wafer parameters"})", "application/json");
                    return;
                }

                const auto id = request.get_param_value("id");
                const auto slotText = request.get_param_value("slot");
                int slot = 0;
                const auto [end, error] = std::from_chars(
                    slotText.data(), slotText.data() + slotText.size(), slot);
                if (id.empty() || error != std::errc{} ||
                    end != slotText.data() + slotText.size() || slot <= 0)
                {
                    response.status = 400;
                    response.set_content(R"({"error":"Invalid wafer parameters"})", "application/json");
                    return;
                }

                if (!controller.registerWafer(Wafer{id, Location{Station::LoadPort, slot}}))
                {
                    response.status = 409;
                    response.set_content(R"({"error":"Wafer ID or location already occupied"})", "application/json");
                    return;
                }

                response.status = 201;
                response.set_content(R"({"registered":true})", "application/json");
            }
        );

        server.Get("/api/status",
            [&controller](const httplib::Request&,
                          httplib::Response& response)
            {
                response.set_content(
                    toJson(controller.status()),
                    "application/json"
                );
            }
        );

        server.Get("/api/events",
            [&controller](const httplib::Request&,
                          httplib::Response& response)
            {
                response.set_content(
                    toJson(controller.events()),
                    "application/json"
                );
            }
        );


server.Post("/api/run",
    [&controller](const httplib::Request& request,
                  httplib::Response& response)
    {
        if (!request.has_param("recipeId"))
        {
            response.status = 400;
            response.set_content(
                R"({"error":"Missing recipe ID"})",
                "application/json"
            );
            return;
        }

        int recipeId;

        try
        {
            const auto idText = request.get_param_value("recipeId");

            std::size_t pos = 0;
            recipeId = std::stoi(idText, &pos);

            if (pos != idText.size() || recipeId <= 0)
            {
                response.status = 400;
                response.set_content(
                    R"({"error":"Invalid recipe ID"})",
                    "application/json"
                );
                return;
            }
        }
        catch (const std::exception&)
        {
            response.status = 400;
            response.set_content(
                R"({"error":"Invalid recipe ID"})",
                "application/json"
            );
            return;
        }

        if (!controller.getRecipe(recipeId))
        {
            response.status = 404;
            response.set_content(
                R"({"error":"Recipe not found"})",
                "application/json"
            );
            return;
        }

	std::uint64_t runId = 0;

        const auto result = controller.runWafer(recipeId, &runId);

        if (result == StartResult::Accepted)
        {
		response.status = 202;

		response.set_content(
    			"{\"accepted\":true,\"runId\":" +
    			std::to_string(runId) + "}",
    			"application/json"
		);
        }
        else
        {
            response.status = 409;
            response.set_content(
                R"({"accepted":false,"error":"Run rejected"})",
                "application/json"
            );
        }
    }
);

server.Post("/api/cancel",
    [&controller](const httplib::Request&,
                  httplib::Response& response)
    {
        const auto result = controller.cancelRun();

        if (result.isCancelled)
        {
            response.status = 202;
            response.set_content(
                R"({"accepted":true})",
                "application/json"
            );
        }
        else
        {
            response.status = 409;
            response.set_content(
                R"({"accepted":false,"error":"No active run to cancel"})",
                "application/json"
            );
        }
    }
);

server.Post("/api/reset",
    [&controller](const httplib::Request&,
                  httplib::Response& response)
    {
        const auto result = controller.resetEquipment();

        switch (result)
        {
            case RecoveryResult::Recovered:
                response.status = 200;
                response.set_content(
                    R"({"recovered":true})",
                    "application/json"
                );
                break;

            case RecoveryResult::NotRequired:
                response.status = 409;
                response.set_content(
                    R"({"recovered":false,"error":"Equipment is not faulted"})",
                    "application/json"
                );
                break;

            case RecoveryResult::Failed:
                response.status = 409;
                response.set_content(
                    R"({"recovered":false,"error":"Equipment recovery failed"})",
                    "application/json"
                );
                break;
        }
    }
);

    }
}

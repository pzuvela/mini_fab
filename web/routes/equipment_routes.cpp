#include "web/routes/equipment_routes.h"

#include "controller/mini_fab_controller.h"
#include "web/serialization/equipment_snapshot_json.h"
#include "web/serialization/equipment_events_json.h"
#include "web/parsing/recipe_parser.h"
#include "engine/start_result.h"

#include <cmath>
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
            const auto idText =
                request.get_param_value("recipeId");

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

        const auto result = controller.runWafer(recipeId);

        if (result == StartResult::Accepted)
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
                R"({"accepted":false,"error":"Run rejected"})",
                "application/json"
            );
        }
    }
);

    }
}

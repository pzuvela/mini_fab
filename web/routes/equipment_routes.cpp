#include "web/routes/equipment_routes.h"

#include "controller/mini_fab_controller.h"
#include "web/serialization/equipment_snapshot_json.h"
#include "web/serialization/equipment_events_json.h"
#include "web/parsing/recipe_parser.h"

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
        auto recipe = parseRecipe(request);

        if (!recipe)
        {
            response.status = 400;
            response.set_content(
                R"({"error":"Invalid recipe parameters"})",
                "application/json"
            );
            return;
        }

        OperationResult result = controller.runWafer(*recipe);

        if (result == OperationResult::Completed)
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
                R"({"accepted":false})",
                "application/json"
            );
        }
    }
);

    }
}

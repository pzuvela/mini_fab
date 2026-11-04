#include <iostream>
#include <string>

#include "httplib.h"
#include "controller/mini_fab_controller.h"
#include "web/serialization/equipment_snapshot_json.h"
#include "web/serialization/equipment_events_json.h"


int main()
{
	MiniFabController controller;
	httplib::Server server;
	std::string ip = "127.0.0.1";
	int port = 8080;

	if (!server.set_mount_point("/", "./web/public"))
	{
    		std::cerr << "Failed to mount web/public\n";
    	return 1;
	}

	server.Get("/api/health",
		[](const httplib::Request&,
			httplib::Response& response)
		{
			response.set_content(
				R"({"status": "ok", "system": "MiniFab"})",
				"application/json"
			);
		}
	);


	server.Get("/api/status",
		[&controller](const httplib::Request&,
				httplib::Response& response)
		{
			response.set_content(
				MiniFab::Web::toJson(controller.status()),
				"application/json"
			);
		}
	);


server.Post("/api/run",
    [&controller](const httplib::Request& request,
                  httplib::Response& response)
    {
        auto badRequest = [&response]()
        {
            response.status = 400;
            response.set_content(
                R"({"error":"Invalid recipe parameters"})",
                "application/json"
            );
        };

        if (!request.has_param("recipeId") ||
            !request.has_param("recipeName") ||
            !request.has_param("durationSeconds"))
        {
            badRequest();
            return;
        }

        Recipe recipe;

        try
        {
            const auto idText = request.get_param_value("recipeId");
            const auto durationText =
                request.get_param_value("durationSeconds");

            std::size_t idPos = 0;
            std::size_t durationPos = 0;

            recipe.id = std::stoi(idText, &idPos);
            recipe.name = request.get_param_value("recipeName");
            recipe.durationSeconds =
                std::stod(durationText, &durationPos);

            if (idPos != idText.size() ||
                durationPos != durationText.size() ||
                recipe.id <= 0 ||
                recipe.name.empty() ||
                !std::isfinite(recipe.durationSeconds) ||
                recipe.durationSeconds <= 0 ||
                recipe.durationSeconds > 3600)
            {
                badRequest();
                return;
            }
        }
        catch (const std::invalid_argument&)
        {
            badRequest();
            return;
        }
        catch (const std::out_of_range&)
        {
            badRequest();
            return;
        }

        OperationResult result = controller.runWafer(recipe);

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

	server.Get("/api/events",
		[&controller](const httplib::Request&,
			httplib::Response& response)
		{
			response.set_content(
				MiniFab::Web::toJson(controller.events()),
				"application/json"
			);
		}
	);

	std::cout << "MiniFab server on port " << port << "\n";

	server.listen(ip, port);
}

